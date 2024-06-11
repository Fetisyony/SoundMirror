#pragma once

#include <stdio.h>
#include <Windows.h>
#include <initguid.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <assert.h>
#include <mmreg.h>
#include <iostream>
#include <string>

#include "file_errors.h"

#define RECORDING_DURATION_SECONDS 5

int initWavFile(WAVEFORMATEX *format);

int writeWavData(BYTE *data, UINT32 newDataSize);

bool is_enough(UINT64 bytes_in_second);

int close_file();
