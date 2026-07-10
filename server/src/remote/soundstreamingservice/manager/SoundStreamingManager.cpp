#include "SoundStreamingManager.hpp"
#include <boost/lockfree/spsc_queue.hpp>

#include "remote/soundstreamingservice/producer/Producer.hpp"
#include "remote/soundstreamingservice/audiobufferpool/AudioBufferPool.hpp"

#include "constants/QueueSizes.hpp"
#include "remote/soundstreamingservice/consumer/Consumer.hpp"
#include "remote/soundstreamingservice/consumerservice/networkstreamingservice/StreamingService.hpp"
#include "remote/soundstreamingservice/consumerservice/wavefileconsumerservice/WaveConsumerService.hpp"
#include "remote/soundstreamingservice/recorder/utils.hpp"


void streamSound() {
    queue_constants::LockfreeAudioQueueType audioQueue;
    auto bufferPool = std::make_shared<AudioBufferPool>(960 * 8, 256);
    std::atomic keepRunning{true};

    auto producer = Producer(audioQueue, bufferPool, keepRunning);
    auto format = producer.initialize();
    printFormat(format);

    auto consumerService = std::make_shared<StreamingService>(ConfigManager::getInstance().getConfig().network.streamingPort);

    auto consumer = Consumer(consumerService, audioQueue, bufferPool, keepRunning);
    consumer.initialize(format);

    consumer.start();
    producer.start();

    spdlog::info("Streaming...");

    producer.join();
    consumer.join();

    consumerService->destroy();
}