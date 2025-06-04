#pragma once
#include <boost/lockfree/spsc_queue.hpp>

struct AudioChunk;

namespace QueueConstants {
    static constexpr size_t RING_CAPACITY = 1024;
    using LockfreeAudioQueueType = boost::lockfree::spsc_queue<AudioChunk *, boost::lockfree::capacity<RING_CAPACITY> >;
}
