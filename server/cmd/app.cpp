#include <boost/lockfree/spsc_queue.hpp>

#include "producer/Producer.hpp"
#include "AudioBufferPool.hpp"
#include "config/ConfigManager.hpp"
#include "constants/QueueSizes.hpp"
#include "consumer/Consumer.hpp"
#include "network/StreamingService.hpp"
#include "timeservice/SyncException.hpp"
#include "timeservice/TimeSyncService.hpp"
#include "wavefile/WaveCreator.hpp"

void launch() {
    const AppConfig &appConfig = ConfigManager::getInstance().getConfig();

    QueueConstants::LockfreeAudioQueueType audioQueue;
    auto bufferPool = std::make_shared<AudioBufferPool>(4096, 256);
    std::atomic keepRunning{true};

    auto syncer = TimeSyncService(ConfigManager::getInstance().getConfig().network.timeSyncPort);
    try {
        syncer.sync();
        std::cout << "[Time synced]" << std::endl;
    } catch (SyncException &e) {
        std::cout << e.what() << std::endl;
        return;
    }

    auto producer = Producer(audioQueue, bufferPool, keepRunning);
    auto format = producer.initialize();

    auto consumerService = std::make_shared<StreamingService>(appConfig.network.streamingPort);
    auto consumer = Consumer(consumerService, audioQueue, bufferPool, keepRunning);
    consumer.initialize(format);

    producer.start();
    consumer.start();

    std::cout << "Running..." << std::endl;
    producer.join();
    consumer.join();
    keepRunning.store(false, std::memory_order_release);
}

int main() {
    ConfigManager::getInstance().initialize("../src/config/config.json");

    while (true) {
        launch();
    }

    return 0;
}
