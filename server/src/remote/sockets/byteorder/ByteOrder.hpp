#pragma once

#include <minwindef.h>

#include "remote/soundstreamingservice/consumer/Consumer.hpp"

namespace byteorder {
    uint32_t bufferLEToUint32(const uint8_t bytes[4]);

    inline void writeLongToBufferLE(BYTE* buf, int64_t val) {
        for (int i = 0; i < 8; ++i) {
            buf[i] = static_cast<BYTE>((val >> (i * 8)) & 0xFF);
        }
    }

    inline unsigned short swapEndianess(unsigned short value) {
        return (value << 8) | (value >> 8);
    }

    int swapSoundEndianess(BYTE *pData, UINT32 numFrames, WAVEFORMATEX *format);
}
