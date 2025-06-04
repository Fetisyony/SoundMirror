#pragma once
#include <winsock2.h>

#include <iostream>
#include <string>

#include <Audioclient.h>
#include <memory>

#include <setupapi.h>

#include "recorder/WASAPIAudioRecorder.hpp"

#include <windows.h>


using namespace std;

typedef unsigned char BYTE;

#define SECONDS_IN_SHARED_BUFFER 10

#define HANDLE_RET_CODE(hr, message, label) if (FAILED(hr)) { \
    cout << "Error: " << message << " - failed (" << (long)hr << ")" << endl; \
    goto label; \
}

#define SAFE_RELEASE(punk) \
    if ((punk) != NULL) {  \
        hr = (punk)->Release(); \
        (punk) = NULL;     \
    }

class WASAPIAudioRecorderBuilder {
public:
    HRESULT initEnumerator();

    HRESULT initializeSharedClient(int secs_in_buffer);

    HRESULT initializeLoopbackRecorder();

    HRESULT initializeExclusiveClient(int secs_in_buffer);

    HRESULT getService();

    HRESULT initializeMicrophoneRecorder();

    /* Simply activates current audioclient.
    Encapsulates many letters. */
    HRESULT activateClient();

    /* Simply gets format from current audioclient.
    Encapsulates many letters. */
    HRESULT initializeSharedModeFormat();

    HRESULT initializeExclusiveModeFormat();

    HRESULT setEventHandle(HANDLE captureEvent);

    void listAudioCaptureDevices();

    std::shared_ptr<WASAPIAudioRecorder> build();

private:
    std::shared_ptr<WASAPIAudioRecorder> _recorder = std::make_shared<WASAPIAudioRecorder>();
};
