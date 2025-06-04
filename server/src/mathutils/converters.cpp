#include "mathutils/converters.hpp"

#include <algorithm>

#include "ErrorCodes.hpp"

unsigned short swapEndianess(unsigned short value) {
    return (value << 8) | (value >> 8);
}

int swapSoundEndianess(BYTE *pData, UINT32 numFrames, WAVEFORMATEX *format) {
    UINT32 numSamples = numFrames * format->nChannels;

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
