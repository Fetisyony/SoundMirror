#include "Consumer.hpp"

#include <memory>
#include <thread>

#include "AudioBufferPool.hpp"

Consumer::Consumer(
    const std::shared_ptr<IConsumerService> &consumerService,
    QueueConstants::LockfreeAudioQueueType &audioQueue,
    const std::shared_ptr<AudioBufferPool> &bufferPool,
    std::atomic<bool> &keepRunning
) : _consumerService(consumerService), _audioQueue(audioQueue), _bufferPool(bufferPool), _keepRunning(keepRunning) {
}

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
}

void Consumer::stop() {
    isThreadRunning = false;

    std::cout << "[Consumer] Thread exiting.\n";
}

void Consumer::runConsuming() {
    AudioChunk *chunkPtr = nullptr;

    while (_keepRunning.load(std::memory_order_acquire)) {
        if (_audioQueue.pop(chunkPtr)) {
            processChunk(chunkPtr);
        } else {
            std::this_thread::sleep_for(1ms);
        }
    }

    while (_audioQueue.pop(chunkPtr)) {
        processChunk(chunkPtr);
    }

    std::cout << "[Consumer] Shutdown complete.\n";
}

void Consumer::processChunk(AudioChunk *chunk) {
    if (!chunk) return;

    auto rc = _consumerService->consumeNewData(chunk->data, chunk->byteCount);
    if (rc != OK)
        _keepRunning.store(false, std::memory_order_release);

    _bufferPool->ReleaseChunk(chunk);
}

void Consumer::join() {
    if (workerThread.joinable()) {
        std::cout << "Consumer: Waiting for worker thread to join..." << std::endl;
        workerThread.join();
        std::cout << "Consumer: Worker thread joined successfully." << std::endl;
    } else {
        std::cout << "Consumer: Worker thread is not joinable (already joined or detached)." << std::endl;
    }
}

Consumer::~Consumer() {
    stop();

    _consumerService->destroy();
    if (workerThread.joinable()) {
        std::cout << "Consumer: Joining worker thread..." << std::endl;
        workerThread.join();
        std::cout << "Consumer: Worker thread joined successfully." << std::endl;
    } else {
        std::cout << "Consumer: Worker thread is not joinable (already joined or detached)." << std::endl;
    }
}
