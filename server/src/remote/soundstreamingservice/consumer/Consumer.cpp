#include "Consumer.hpp"

#include <iostream>
#include <memory>
#include <thread>

#include <chrono>

#include "remote/soundstreamingservice/ControlCodes.hpp"
using namespace std::chrono_literals;

#include "remote/soundstreamingservice/Common.hpp"
#include "remote/soundstreamingservice/audiobufferpool/AudioBufferPool.hpp"
#include "spdlog/spdlog.h"


Consumer::Consumer(
    const std::shared_ptr<IConsumerService> &consumerService,
    queue_constants::LockfreeAudioQueueType &audioQueue,
    const std::shared_ptr<AudioBufferPool> &bufferPool,
    std::atomic<bool> &keepRunning
) : _consumerService(consumerService),
    _audioQueue(audioQueue),
    _bufferPool(bufferPool),
    _keepRunning(keepRunning) {}

void Consumer::initialize(WAVEFORMATEX *format) {
    if (isInitialized) {
        throw std::runtime_error("Already initialized");
    }

    _consumerService->initialize(format);
    isInitialized = true;
}

void Consumer::start() {
    if (!isInitialized) {
        throw std::runtime_error("Initialize first");
    }
    if (isThreadRunning) {
        throw std::runtime_error("Already running");
    }

    isThreadRunning = true;
    workerThread = std::thread([this] {
        runConsuming();
    });

    checkerThread = std::thread([this] {
        runChecking();
    });
}

void Consumer::stop() {
    isThreadRunning = false;

    spdlog::info("[Consumer] Thread exiting.");
}

void Consumer::runChecking() {
    while (_keepRunning.load(std::memory_order_acquire)) {
        int value = _consumerService->checkAction();
        if (value > 0) {
            spdlog::info("[Consumer] Action returned: {}", value);
            if (value == SoundStreamingControlCode::STOP_STREAMING) {
                _keepRunning.store(false, std::memory_order_release);
            }
        } else {
            spdlog::warn("[Consumer] Action returned: {}. Exiting...", value);
            _keepRunning.store(false, std::memory_order_release);
        }
    }
}

void Consumer::runConsuming() {
    AudioChunk *chunkPtr = nullptr;

    while (_keepRunning.load(std::memory_order_acquire)) {
        // Wait until queue is not empty or external stop signal
        while (_audioQueue.pop(chunkPtr)) {
            processChunk(chunkPtr);
        }
        std::unique_lock lk(common::consumerProducerMutex);
        common::cv.wait(lk, [&]{
            return !_keepRunning.load() || _audioQueue.read_available() > 0;
        });
    }
    while (_audioQueue.pop(chunkPtr)) {
        processChunk(chunkPtr);
    }

    spdlog::info("[Consumer] Shutdown complete.");
}

void Consumer::processChunk(AudioChunk *chunk) {
    if (!chunk) return;

    auto rc = _consumerService->consumeNewData(chunk->data, chunk->byteCount);
    if (rc != OK)
        _keepRunning.store(false, std::memory_order_release);
    // spdlog::info("Sent chunk");

    _bufferPool->ReleaseChunk(chunk);
}

void Consumer::join() {
    if (workerThread.joinable()) {
        spdlog::info("[Consumer] Waiting for worker thread to join...");
        workerThread.join();
        spdlog::info("[Consumer] Joined worker thread successfully");
    } else {
        spdlog::error("[Consumer] Worker thread is not joinable");
    }

    if (checkerThread.joinable()) {
        spdlog::info("[Consumer] Waiting for checker thread to join...");
        checkerThread.join();
        spdlog::info("[Consumer] Joined checker thread successfully");
    } else {
        spdlog::error("[Consumer] Checker thread is not joinable");
    }
}

Consumer::~Consumer() {
    stop();
}
