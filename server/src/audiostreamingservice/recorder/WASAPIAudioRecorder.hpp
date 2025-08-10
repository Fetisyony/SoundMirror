#pragma once

#include <winsock2.h>

#include <iostream>
#include <string>

#include <Audioclient.h>
#include <mmdeviceapi.h>

#include <setupapi.h>

#include <windows.h>

using namespace std;

typedef unsigned char BYTE;

#define SECONDS_IN_SHARED_BUFFER 10

#define SAFE_RELEASE(punk) \
    if ((punk) != NULL) {  \
        hr = (punk)->Release(); \
        (punk) = NULL;     \
    }

class WASAPIAudioRecorder {
    friend class WASAPIAudioRecorderBuilder;

public:
    ~WASAPIAudioRecorder();

    WAVEFORMATEX *getFormat();

    /* Retrieves at least data chunk that lasts for chunkSeconds,
    Puts all data to destBuffer, saves overall number of saved bytes to totalReceived.
    Keeps in mind and in any case it can't overflow a destBuffer, that is bufferLimit in size.
    */
    void collectSound(BYTE *destBuffer, UINT64 &bytesReceived, UINT64 bufferLimit);

    bool isEnough(UINT64 received, UINT64 bytesInSecond, long double chunkSeconds);

    [[nodiscard]] UINT64 getBytesInSecond() const;

    void startRecording();

private:
    IMMDeviceEnumerator *enumerator = nullptr;
    IMMDevice *recorder = nullptr;
    IAudioClient *pAudioClient = nullptr;
    IAudioCaptureClient *pCaptureClient = nullptr;
    WAVEFORMATEX *format = nullptr;

    // variables passed to GetBuffer when collecting recorded sound
    UINT32 nFrames{};
    DWORD flags{};
    BYTE *captureBuffer{};
    UINT32 packetLength = 0;
};
