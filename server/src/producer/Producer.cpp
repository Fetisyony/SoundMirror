#include "Producer.hpp"

#include <atomic>

#include "AudioBufferPool.hpp"
#include "avrt.h"
#include "ErrorCodes.hpp"
#include "constants/QueueSizes.hpp"
#include "recorder/creators/directors/sharedmode/WASAPIAudioSharedModeRecorderDirector.hpp"
#include "recorder/exceptions/EventCreationFailureException.hpp"

#define CONTROL_THREAD_PRIORITY_SETTINGS

inline int64_t getCurrentTimestamp() {
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    return qpc.QuadPart;
}

Producer::Producer(QueueConstants::LockfreeAudioQueueType &audioQueue, const std::shared_ptr<AudioBufferPool> &bufferPool,
                   std::atomic<bool> &keepRunning) : _audioQueue(audioQueue), _bufferPool(bufferPool), _keepRunning(keepRunning) {
}

WAVEFORMATEX *Producer::initialize() {
    if (isInitialized) {
        throw std::runtime_error("Already initialized");
    }

    _captureEvent = CreateEvent(
        /*lpEventAttributes=*/ nullptr,
                               /*bManualReset=*/ FALSE,
                               // - FALSE (auto-reset): once signaled and a thread wakes, it automatically resets to non-signaled.
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
        std::cout << "[Producer] Warning: Failed to set MMCSS characteristics.\n";
    }
#endif
}

void Producer::resetThreadSettings() {
#ifdef CONTROL_THREAD_PRIORITY_SETTINGS
    if (_mmTask) {
        AvRevertMmThreadCharacteristics(_mmTask);
    }
#endif
}

void Producer::runProduction() {
    _recorder->startRecording();

    while (_keepRunning.load(std::memory_order_acquire)) {
        DWORD waitResult = WaitForSingleObject(_captureEvent, INFINITE);
        if (waitResult != WAIT_OBJECT_0) {
            if (!_keepRunning.load()) break;
            std::cerr << "WaitForSingleObject returned " << waitResult << "\n";
            break;
        }

        AudioChunk *chunk = _bufferPool->AcquireChunk();

        if (chunk) {
            _recorder->collectSound(chunk->data, chunk->byteCount, chunk->capacity);
            chunk->timestamp = getCurrentTimestamp();

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
        }
    }
}

void Producer::stop() {
    isThreadRunning = false;

    std::cout << "[Producer] Capture thread exiting.\n";
}

Producer::~Producer() {
    stop();
    if (workerThread.joinable()) {
        workerThread.join();
    }
}
