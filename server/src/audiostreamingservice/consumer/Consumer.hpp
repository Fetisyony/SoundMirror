#pragma once
#include <atomic>
#include <memory>
#include <thread>

#include "audiostreamingservice/audiobufferpool/AudioBufferPool.hpp"
#include "ErrorCodes.hpp"
#include "IConsumerService.hpp"
#include "constants/QueueSizes.hpp"

class Consumer {
public:
    Consumer(
        const std::shared_ptr<IConsumerService> &consumerService,
        QueueConstants::LockfreeAudioQueueType &audioQueue,
        const std::shared_ptr<AudioBufferPool> &bufferPool,
        std::atomic<bool> &keepRunning
    );

    void initialize(WAVEFORMATEX *format);

    void start();

    void stop();

    void join();

    ~Consumer();

private:
    std::shared_ptr<IConsumerService> _consumerService;
    QueueConstants::LockfreeAudioQueueType &_audioQueue;
    std::shared_ptr<AudioBufferPool> _bufferPool;
    std::atomic<bool> &_keepRunning;

    std::atomic<bool> isThreadRunning;
    std::atomic<bool> isInitialized;
    std::thread workerThread;

    void runConsuming();

    void processChunk(AudioChunk *chunk);
};
