#pragma once

#include <stdio.h>
#include <iostream>
#include <string>

#include "server.h"
#include <windows.h>
#include <initguid.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <mmreg.h>

#include "wave_file_controller.h"

// Define the length of audio capture in seconds
#define SECONDS_IN_SHARED_BUFFER 5

#define HANDLE_ERROR(hr, message, label) if (FAILED(hr)) { \
    std::cout << "Error: " << message << "- failed (" << hr << ")" << std::endl; \
    goto label; \
}

#define SAFE_RELEASE(punk) \
    if ((punk) != NULL) {  \
        hr = (punk)->Release(); \
        (punk) = NULL;     \
    }

HRESULT check_error(HRESULT hres, std::string message);

int run_wav_recording();

void print_format(WAVEFORMATEX *format);

HRESULT init_client(IAudioClient *pAudioClient, WAVEFORMATEX *format, int secs_in_buffer);

HRESULT capture_sound(IAudioClient *pAudioClient, IAudioCaptureClient *&pCaptureClient, WAVEFORMATEX *format, int init_data(WAVEFORMATEX *format), int (*proc_data)(BYTE *captureBuffer, UINT32 bytes_captured), int (*finish)(void), bool (*enough)(UINT64 bytes_in_second));

int init_capturer(IMMDeviceEnumerator* &enumerator, IMMDevice* &recorder, IAudioClient* &pAudioClient, WAVEFORMATEX* &format);
