#pragma once

#include <Audioclient.h>

unsigned short swapEndianess(unsigned short value);

int swapSoundEndianess(BYTE *pData, UINT32 numFrames, WAVEFORMATEX *format);
