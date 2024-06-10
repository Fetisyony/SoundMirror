#include <stdio.h>
#include <Windows.h>
#include <initguid.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <assert.h>
#include <mmreg.h>
#include <iostream>
#include <string>

#define OK 0

// Define the length of audio capture in seconds
#define RECORDING_DURATION_SECONDS 10
#define SECONDS_IN_SHARED_BUFFER 10

#define FILE_ERROR 100

#define SAFE_RELEASE(punk) \
    if ((punk) != NULL) {  \
        hr = (punk)->Release(); \
        (punk) = NULL;     \
    }

HRESULT check_error(HRESULT hres, std::string message);

void print_format(WAVEFORMATEX *format);

void initWavFile(FILE *file, WAVEFORMATEX *format);

void writeWavData(FILE *file, BYTE *data, int oldDataSize, int newDataSize);

HRESULT init_client(IAudioClient *pAudioClient, WAVEFORMATEX *format, int secs_in_buffer);

HRESULT run_service(IAudioClient *pAudioClient, IAudioCaptureClient *&pCaptureClient, WAVEFORMATEX *format);

int init_capturer(IMMDeviceEnumerator* &enumerator, IMMDevice* &recorder, IAudioClient* &pAudioClient, WAVEFORMATEX* &format);
