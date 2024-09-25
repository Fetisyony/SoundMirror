#pragma once

#include <stdio.h>
#include <iostream>
#include <windows.h>

#include <mmreg.h>

#include "../errors.h"

#define ERROR_FILE 31

#define RECORDING_DURATION_SECONDS 5

int initWavFile(WAVEFORMATEX *format);

int writeWavData(BYTE *data, UINT32 new_data_size, WAVEFORMATEX *format);

bool is_enough(UINT64 bytesInSecond);

int close_file();
