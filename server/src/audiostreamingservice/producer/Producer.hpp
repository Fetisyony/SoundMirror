#pragma once
#include <atomic>
#include <memory>
#include <thread>

#include "constants/QueueSizes.hpp"
#include "audiostreamingservice/recorder/WASAPIAudioRecorder.hpp"
#include "audiostreamingservice/audiobufferpool/AudioBufferPool.hpp"

class Producer {
public:
    Producer(
        QueueConstants::LockfreeAudioQueueType &audioQueue,
        const std::shared_ptr<AudioBufferPool> &bufferPool,
        std::atomic<bool> &keepRunning
    );

    WAVEFORMATEX *initialize();

    void start();

    void stop();

    void join();

    ~Producer();

private:
    std::shared_ptr<WASAPIAudioRecorder> _recorder{};
    QueueConstants::LockfreeAudioQueueType &_audioQueue;
    std::shared_ptr<AudioBufferPool> _bufferPool;
    std::atomic<bool> &_keepRunning;

    HANDLE _captureEvent{};

    HANDLE _mmTask{};
    std::atomic<bool> isThreadRunning;
    std::atomic<bool> isInitialized;
    std::thread workerThread;

    void runProduction();

    void configureThread();
    void resetThreadConfigurations();
};
