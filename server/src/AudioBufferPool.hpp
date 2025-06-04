#pragma once
#include <cstdint>
#include <vector>
#include <boost/lockfree/spsc_queue.hpp>

struct AudioChunk {
    uint8_t *data;
    size_t byteCount;
    int64_t timestamp;
    size_t capacity;
};

class AudioBufferPool {
public:
    AudioBufferPool(size_t chunkSize, size_t poolSize);

    ~AudioBufferPool();

    AudioChunk *AcquireChunk();

    void ReleaseChunk(AudioChunk *chunk);

private:
    std::vector<AudioChunk> _pool;
    boost::lockfree::spsc_queue<size_t, boost::lockfree::capacity<1024> > _freeIndices;
};
