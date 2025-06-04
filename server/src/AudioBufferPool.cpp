#include "AudioBufferPool.hpp"

#include <cstdlib>


AudioBufferPool::AudioBufferPool(size_t chunkSize, size_t poolSize) {
    _pool.reserve(poolSize);
    for (size_t i = 0; i < poolSize; ++i) {
        auto ptr = static_cast<uint8_t *>(_aligned_malloc(chunkSize, 16));
        _pool.push_back({ptr, 0, 0, chunkSize});
        _freeIndices.push(i);
    }
}

AudioBufferPool::~AudioBufferPool() {
    for (auto &item: _pool) {
        _aligned_free(item.data);
    }
}

AudioChunk *AudioBufferPool::AcquireChunk() {
    size_t idx;
    if (!_freeIndices.pop(idx)) return nullptr;

    AudioChunk *chunkPtr = &_pool[idx];
    chunkPtr->byteCount = 0;
    chunkPtr->timestamp = 0;
    return chunkPtr;
}

void AudioBufferPool::ReleaseChunk(AudioChunk *chunk) {
    if (!chunk) return;

    size_t idx = chunk - _pool.data();
    _freeIndices.push(idx);
}
