#pragma once

#include <stdio.h>
#include <iostream>
#include <windows.h>

#include <mmreg.h>

#include "../ErrorsConfig.h"

using std::cout;
using std::endl;

class WaveCreator {
    const char *filename = "test.wav";

    FILE *file = NULL;
    UINT64 written_data_size = 0;
    
    WAVEFORMATEX *format;
        
    DWORD cksize = sizeof(WAVEFORMATEXTENSIBLE);
    int HEADER_SIZE = 4 + 4 + 4 + 4 + 4 + cksize + 4 + 4;  // 68

public:
    WaveCreator(WAVEFORMATEX *&format);

    int initWaveFile();

    // Adds new data in bytes to the initialized wave file
    int addSound(BYTE *data, UINT32 new_data_size);

    bool isEnough(UINT64 bytesInSecond, double secondsNeed);

    int closeFile();
};
