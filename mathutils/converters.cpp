#include "converters.h"

unsigned short swap_endianess(unsigned short value) {
    return (value << 8) | (value >> 8);
}

int swap_sound_endianess(BYTE *pData, UINT32 numFrames, WAVEFORMATEX *pwfx) {
    UINT32 numSamples = numFrames * pwfx->nChannels;

    switch (pwfx->wBitsPerSample) {
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
    }

    return OK;
}
