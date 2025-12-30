#include "ByteOrder.hpp"

#include "ErrorCodes.hpp"

namespace byteorder {
    uint32_t bufferLEToUint32(const uint8_t bytes[4]) {
        return static_cast<uint32_t>(bytes[0]) |
               (static_cast<uint32_t>(bytes[1]) << 8) |
               (static_cast<uint32_t>(bytes[2]) << 16) |
               (static_cast<uint32_t>(bytes[3]) << 24);
    }

    int swapSoundEndianess(BYTE *pData, UINT32 numFrames, WAVEFORMATEX *format) {
        const UINT32 numSamples = numFrames * format->nChannels;

        switch (format->wBitsPerSample) {
            case 16:
                for (UINT32 i = 0; i < numSamples; ++i) {
                    std::swap(pData[i * 2], pData[i * 2 + 1]);
                }
                break;
            case 24:
                for (UINT32 i = 0; i < numSamples; ++i) {
                    std::swap(pData[i * 3], pData[i * 3 + 2]);
                }
                break;
            case 32:
                for (UINT32 i = 0; i < numSamples; ++i) {
                    std::swap(pData[i * 4], pData[i * 4 + 3]);
                    std::swap(pData[i * 4 + 1], pData[i * 4 + 2]);
                }
                break;
            default:
                break;
        }

        return OK;
    }
}
