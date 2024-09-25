#pragma once

#include <Audioclient.h>
#include <algorithm>
#include "../errors.h"

unsigned short swap_endianess(unsigned short value);

int swap_sound_endianess(BYTE *pData, UINT32 numFrames, WAVEFORMATEX *pwfx);
