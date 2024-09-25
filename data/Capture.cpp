#include "Capture.h"


bool isEnough(UINT64 received, UINT64 bytesInSecond, double chunkSeconds) {
    return received >= bytesInSecond * chunkSeconds;
}

UINT64 Capture::getBytesInSecond() {
    return bytesInSecond;
}

int Capture::startSoundCaptureWrapper() {
    HRESULT hr = startSoundCapture();

    if (FAILED(hr))
        return ERROR_START_CAPTURE;
    return OK;
}

HRESULT Capture::startSoundCapture() {
    HRESULT hr = OK;

    hr = start();
    HANDLE_RET_CODE(hr, "start", done);

    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    HANDLE_RET_CODE(hr, "GetService", done);
    hr = pAudioClient->Start();
    HANDLE_RET_CODE(hr, "Start", done);

    bytesInSecond = format->nSamplesPerSec * (format->wBitsPerSample / 8) * format->nChannels;

done:
    showWasapiErrorMessage(hr);
    return hr;
}

HRESULT Capture::collectSound(double chunkSeconds, BYTE *destBuffer, UINT64 *totalReceived, UINT64 bufferLimit) {
    HRESULT hr = OK;

    UINT64 received = 0;

    hr = pCaptureClient->GetNextPacketSize(&packetLength);
    HANDLE_RET_CODE(hr, "GetNextPacketSize", done);

    while (runCollecting && !isEnough(received, bytesInSecond, chunkSeconds) && received < bufferLimit) {
        hr = pCaptureClient->GetBuffer(&captureBuffer, &nFrames, &flags, NULL, NULL);
        HANDLE_RET_CODE(hr, "GetBuffer", done);

        UINT64 nMaxFrames = min((bufferLimit - received) / format->nBlockAlign, (UINT64)nFrames);
        UINT64 nMaxBytes = nMaxFrames * format->nBlockAlign;
        // n <= (bufferLimit - received) / format->nBlockAlign
        // max {n: n <= nFrames}: n * format->nBlockAlign <= bufferLimit
        received += nMaxBytes;
        memcpy(destBuffer, captureBuffer, nMaxBytes);
        destBuffer += nMaxBytes;

        // well, we took frames that we can accept, let's just skip ones that left
        hr = pCaptureClient->ReleaseBuffer(nFrames);
        HANDLE_RET_CODE(hr, "ReleaseBuffer", done);

        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        HANDLE_RET_CODE(hr, "GetNextPacketSize", done);
    }
    cout << "[collectSound] Collected: " << received << endl;
    if (SUCCEEDED(hr))
        *totalReceived = received;

done:
    return hr;
}

Capture::Capture() {
    HRESULT hr = OK;

    hr = performOverview();
    if (FAILED(hr)) {
        throw std::runtime_error("Unable to initialize Capture!"); 
    }
}

void Capture::getFormat(WAVEFORMATEX **format) {
    *format = this->format;
}

HRESULT Capture::performOverview() {
    HRESULT hr = OK;

    // Initializes the COM library for use by the calling thread,
    // sets the thread's concurrency model, and creates a new apartment for the thread if one is required.
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HANDLE_RET_CODE(hr, "CoInitializeEx", done);

    hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        IID_IMMDeviceEnumerator,
        (void**)&enumerator
    );
    HANDLE_RET_CODE(hr, "CoCreateInstance", done);

    // show available devices
    IMMDeviceCollection *collection;
    hr = enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection);
    HANDLE_RET_CODE(hr, "EnumAudioEndpoints", done);
    // print them
    UINT count;
    hr = collection->GetCount(&count);
    HANDLE_RET_CODE(hr, "GetCount", done);
    cout << "Available devices number: " << count << endl;

done:
    return hr;
}

HRESULT Capture::start() {
    HRESULT hr = OK;
    
    if (loopback) {
        hr = initializeLoopbackRecorder();
        HANDLE_RET_CODE(hr, "initializeLoopbackRecorder", done);

        hr = initializeSharedClient(SECONDS_IN_SHARED_BUFFER);
        HANDLE_RET_CODE(hr, "initializeSharedClient", done);
    } else {
        hr = initializeMicrophoneRecorder();
        HANDLE_RET_CODE(hr, "initializeMicrophoneRecorder", done);

        hr = initializeExclusiveClient(SECONDS_IN_SHARED_BUFFER);
        HANDLE_RET_CODE(hr, "initializeExclusiveClient", done);
    }
    printFormat(format);

done:
    return hr;
}

void Capture::printFormat(WAVEFORMATEX *format) {
    printf("Frame size     : %d\n" , format->nBlockAlign);
    printf("Channels       : %d\n" , format->nChannels);
    printf("Bits per second: %d\n" , format->wBitsPerSample);
    printf("Sample rate:   : %ld\n", format->nSamplesPerSec);
    printf("Format:        : %d\n" , format->wFormatTag);
    printf("Size:          : %d\n" , format->cbSize);
}

HRESULT Capture::initializeMicrophoneRecorder() {
    HRESULT hr = OK;

    hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &recorder);
    HANDLE_RET_CODE(hr, "GetDefaultAudioEndpoint", done);

    hr = activateClient();
    HANDLE_RET_CODE(hr, "activateClient", done);

    hr = initializeFormat();
    HANDLE_RET_CODE(hr, "initializeFormat", done);

done:
    return hr;
}

HRESULT Capture::activateClient() {
    HRESULT hr = OK;

    hr = recorder->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    HANDLE_RET_CODE(hr, "Activate", done);

done:
    return hr;
}

HRESULT Capture::initializeFormat() {
    HRESULT hr = OK;

    hr = pAudioClient->GetMixFormat(&format);
    HANDLE_RET_CODE(hr, "GetMixFormat", done);

done:
    return hr;
}

HRESULT Capture::initializeLoopbackRecorder() {
    HRESULT hr = OK;

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &recorder);
    HANDLE_RET_CODE(hr, "GetDefaultAudioEndpoint", done);

    hr = activateClient();
    HANDLE_RET_CODE(hr, "GetDefaultAudioEndpoint", done);

    hr = initializeFormat();
    HANDLE_RET_CODE(hr, "GetDefaultAudioEndpoint", done);

done:
    return hr;
}

HRESULT Capture::initializeExclusiveClient(int secs_in_buffer) {
    HRESULT hr = OK;

    REFERENCE_TIME hnsBufferDuration = 10 * 1000 * 1000 * secs_in_buffer;  // hecto nanoseconds
    hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            0,
            hnsBufferDuration,
            0,  // hnsPeriodicity
            format,
            NULL
        );
    // pAudioClient->GetDevicePeriod(&pDefaultPeriod, &pMinimumPeriod);
    // will give 100000 and 30000 - 10 ms and 3 ms
    // pAudioClient->GetBufferSize(&tmp);  // 480000

    return hr;
}

HRESULT Capture::initializeSharedClient(int secs_in_buffer) {
    HRESULT hr = OK;

    REFERENCE_TIME hnsBufferDuration = 10 * 1000 * 1000 * secs_in_buffer;  // hecto nanoseconds
    hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK,
            hnsBufferDuration,
            0,  // hnsPeriodicity - always 0 in shared mode
            format,
            NULL
        );
    // pAudioClient->GetDevicePeriod(&pDefaultPeriod, &pMinimumPeriod);
    // will give 100000 and 30000 - 10 ms and 3 ms

    // pAudioClient->GetBufferSize(&tmp);  // 480000

    return hr;
}

Capture::~Capture() {
    HRESULT hr = OK;

    pAudioClient->Stop();
    SAFE_RELEASE(enumerator)
    CoTaskMemFree(format);
    SAFE_RELEASE(pCaptureClient)
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(recorder)

    CoUninitialize();

    cout << "Closed capturer with error code: " << hr << endl;
}
