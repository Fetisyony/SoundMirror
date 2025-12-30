#include "Producer.hpp"

#include <atomic>

#include "remote/soundstreamingservice/audiobufferpool/AudioBufferPool.hpp"
#include "avrt.h"
#include "ErrorCodes.hpp"
#include "config/ConfigManager.hpp"
#include "constants/QueueSizes.hpp"
#include "remote/soundstreamingservice/recorder/creators/directors/sharedmode/WASAPIAudioSharedModeRecorderDirector.hpp"
#include "remote/soundstreamingservice/recorder/exceptions/EventCreationFailureException.hpp"
#include "remote/timesyncservice/GetTimestamp.hpp"
#include "remote/soundstreamingservice/Common.hpp"
#include "remote/timesyncservice/TimeSyncService.hpp"

#define CONTROL_THREAD_PRIORITY_SETTINGS

Producer::Producer(queue_constants::LockfreeAudioQueueType &audioQueue,
                   const std::shared_ptr<AudioBufferPool> &bufferPool,
                   std::atomic<bool> &keepRunning) : _audioQueue(audioQueue), _bufferPool(bufferPool),
                                                     _keepRunning(keepRunning) {
}

WAVEFORMATEX *Producer::initialize() {
    if (isInitialized) {
        throw std::runtime_error("Already initialized");
    }

    _captureEvent = CreateEvent(
        /*lpEventAttributes=*/ nullptr,
       /*bManualReset=*/ FALSE, // - FALSE (auto-reset): once signaled and a thread wakes, it automatically resets to non-signaled.
       /*bInitialState=*/ FALSE, // - FALSE (initial state): start as non-signaled.
       /*lpName=*/ nullptr
    );
    if (!_captureEvent) {
        throw EventCreationFailureException(__FILE__, __FUNCTION__, __LINE__,
                                            ("CreateEvent failed: " + std::to_string(GetLastError())).c_str());
    }

    auto director = WASAPIAudioSharedModeRecorderDirector(_captureEvent);
    _recorder = director.create();

    isInitialized = true;
    return _recorder->getFormat();
}

void Producer::start() {
    if (!isInitialized) {
        throw std::runtime_error("Initialize first");
    }
    if (isThreadRunning) {
        throw std::runtime_error("Already running");
    }

    _startTime = get_time_service::getCurrentTimeMs();

    isThreadRunning = true;
    workerThread = std::thread([this] {
        configureThread();
        runProduction();
    });
}

void Producer::configureThread() {
#ifdef CONTROL_THREAD_PRIORITY_SETTINGS
    DWORD taskIndex = 0;
    _mmTask = AvSetMmThreadCharacteristics("Pro Audio", &taskIndex);
    if (_mmTask != nullptr) {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    } else {
        spdlog::warn("[Producer] Failed to set MMCSS characteristics.");
    }
#endif
}

void Producer::resetThreadConfigurations() {
#ifdef CONTROL_THREAD_PRIORITY_SETTINGS
    if (_mmTask) {
        AvRevertMmThreadCharacteristics(_mmTask);
    }
#endif
}

void Producer::runProduction() {
    _recorder->startRecording();

    int durationLimit = ConfigManager::getInstance().getConfig().durationMilliseconds;

    while (_keepRunning.load(std::memory_order_acquire)) {
        DWORD waitResult = WaitForSingleObject(_captureEvent, 100);

        if (waitResult == WAIT_TIMEOUT) {
            continue;
        }
        if (waitResult != WAIT_OBJECT_0) {
            if (!_keepRunning.load()) break;
            spdlog::error("WaitForSingleObject returned {}", waitResult);
            break;
        }

        AudioChunk *chunk = _bufferPool->AcquireChunk();
        if (!chunk) {
            spdlog::error("Couldn't acquire a chunk");
            continue;
        }

        _recorder->collectSound(chunk->data, chunk->byteCount, chunk->capacity);
        if (chunk->byteCount == 0) {
            _bufferPool->ReleaseChunk(chunk);
            continue;
        }
        // chunk->timestamp = sync_data::timestampMilliseconds +
        //     get_time_service::getMillisecondsBetweenTicks(
        //         sync_data::ticks, get_time_service::getCurrentTicksCount(), sync_data::frequency);

        if (!_audioQueue.push(chunk)) {
            // Queue is full: dropping the oldest chunk
            AudioChunk *dropped = nullptr;
            if (_audioQueue.pop(dropped)) {
                _bufferPool->ReleaseChunk(dropped);
                if (!_audioQueue.push(chunk)) {
                    // Still failed (very unlikely). Just dropping
                    _bufferPool->ReleaseChunk(chunk);
                }
            } else {
                // Couldn't pop (what?), just drop this chunk:
                _bufferPool->ReleaseChunk(chunk);
            }
        }
        common::cv.notify_one();

        if (durationLimit != -1 && get_time_service::getCurrentTimeMs() - _startTime > durationLimit) {
            _keepRunning.store(false, std::memory_order_release);
        }
    }
}

void Producer::stop() {
    isThreadRunning = false;

    spdlog::info("[Producer] Stop");
}

void Producer::join() {
    if (workerThread.joinable()) {
        spdlog::info("[Producer] Waiting for worker thread to join...");
        workerThread.join();
        spdlog::info("[Producer] Joined worker thread successfully");
    } else {
        spdlog::error("[Producer] Worker thread is not joinable");
    }
}

Producer::~Producer() {
    stop();

    if (workerThread.joinable()) {
        workerThread.detach();
        spdlog::warn("Producer: Worker thread was still joinable in destructor. Detached.");
    }
}
