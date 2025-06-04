#include <boost/lockfree/spsc_queue.hpp>

#include "AudioBufferPool.hpp"
#include "constants/QueueSizes.hpp"
#include "consumer/Consumer.hpp"
#include "network/StreamingService.hpp"
#include "producer/Producer.hpp"
#include "wavefile/WaveCreator.hpp"

int main() {
    QueueConstants::LockfreeAudioQueueType audioQueue;
    auto bufferPool = std::make_shared<AudioBufferPool>(4096, 256);
    std::atomic keepRunning{true};

    auto producer = Producer(audioQueue, bufferPool, keepRunning);
    auto format = producer.initialize();

    auto consumerService = std::make_shared<StreamingService>(20000);
    auto consumer = Consumer(consumerService, audioQueue, bufferPool, keepRunning);
    consumer.initialize(format);

    producer.start();
    consumer.start();

    std::cout << "Running..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3600));
    keepRunning.store(false, std::memory_order_release);

    return 0;
}
