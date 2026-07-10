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

    inline unsigned short swapEndianess16(unsigned short value) {
        return (value << 8) | (value >> 8);
    }

    inline uint32_t swapEndianess32(uint32_t value) {
        return (value >> 24) |
               (value << 24) |
               ((value >> 8) & 0x0000FF00) |
               ((value << 8) & 0x00FF0000);
    }

    inline uint64_t swapEndianess64(uint64_t value) {
        return ((value >> 0) & 0xFFULL) << 56 |
               ((value >> 8) & 0xFFULL) << 48 |
               ((value >> 16) & 0xFFULL) << 40 |
               ((value >> 24) & 0xFFULL) << 32 |
               ((value >> 32) & 0xFFULL) << 24 |
               ((value >> 40) & 0xFFULL) << 16 |
               ((value >> 48) & 0xFFULL) << 8 |
               ((value >> 56) & 0xFFULL) << 0;
    }

    int swapSoundEndianess(BYTE *pData, UINT32 numFrames, WAVEFORMATEX *format);
}
