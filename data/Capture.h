#pragma once

#include <iostream>
#include <string>
#include <queue>

#include <winsock2.h>
#include <windows.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <mmreg.h>

#include <functiondiscoverykeys.h>
#include <setupapi.h>
#include <initguid.h>  // Put this in to get rid of linker errors.
#include <devpkey.h>
#include <propkey.h>
#include <propvarutil.h>

#include "errors.h"

using namespace std;

typedef unsigned char BYTE;

#define SECONDS_IN_SHARED_BUFFER 0.5

#define HANDLE_RET_CODE(hr, message, label) if (FAILED(hr)) { \
    cout << "Error: " << message << " - failed (" << (long)hr << ")" << endl; \
    goto label; \
}

#define SAFE_RELEASE(punk) \
    if ((punk) != NULL) {  \
        hr = (punk)->Release(); \
        (punk) = NULL;     \
    }

class Capture {
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *recorder = NULL;
    IAudioClient *pAudioClient = NULL;
    IAudioCaptureClient *pCaptureClient = NULL;
    WAVEFORMATEX *format = NULL;

    bool stopThreads = false;

    UINT32 nFrames;
    DWORD flags;
    BYTE *captureBuffer;
    UINT32 packetLength = 0;

    UINT64 bytes_in_second;

public:
    Capture();
    ~Capture();

    void getFormat(WAVEFORMATEX **format);

    HRESULT initializeSharedClient(int secs_in_buffer);

    HRESULT initializeExclusiveClient(int secs_in_buffer);

    HRESULT collectSound(double chunkSeconds, BYTE *destBuffer);

    HRESULT startSoundCapture();

    HRESULT initializeLoopbackRecorder();

    HRESULT initializeMicrophoneRecorder();

    void print_format(WAVEFORMATEX *format);

    HRESULT performOverview();

    /* Simply activates current audioclient.
    Encapsulates scary call. */
    HRESULT Capture::activateClient();

    /* Simply gets format from current audioclient.
    Encapsulates scary call. */
    HRESULT Capture::initializeFormat();

    HRESULT start();
};
