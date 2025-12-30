#pragma once
#include <atomic>
#include <memory>
#include <thread>

#include "remote/soundstreamingservice/audiobufferpool/AudioBufferPool.hpp"
#include "ErrorCodes.hpp"
#include "remote/soundstreamingservice/consumerservice/IConsumerService.hpp"
#include "constants/QueueSizes.hpp"

class Consumer {
public:
    Consumer(
        const std::shared_ptr<IConsumerService> &consumerService,
        queue_constants::LockfreeAudioQueueType &audioQueue,
        const std::shared_ptr<AudioBufferPool> &bufferPool,
        std::atomic<bool> &keepRunning
    );

    void initialize(WAVEFORMATEX *format);

    void start();

    void stop();

    void runChecking();

    void join();

    ~Consumer();

private:
    std::shared_ptr<IConsumerService> _consumerService;
    queue_constants::LockfreeAudioQueueType &_audioQueue;
    std::shared_ptr<AudioBufferPool> _bufferPool;
    std::atomic<bool> &_keepRunning;

    std::atomic<bool> isThreadRunning;
    std::atomic<bool> isInitialized;
    std::thread workerThread;
    std::thread checkerThread;

    void runConsuming();

    void processChunk(AudioChunk *chunk);
};
