#include "capturer.h"

void print_format(WAVEFORMATEX *format) {
    printf("Frame size     : %d\n" , format->nBlockAlign);
    printf("Channels       : %d\n" , format->nChannels);
    printf("Bits per second: %d\n" , format->wBitsPerSample);
    printf("Sample rate:   : %ld\n", format->nSamplesPerSec);
    printf("Format:        : %d\n" , format->wFormatTag);
    printf("Size:          : %d\n" , format->cbSize);
}

int swap_sound_endianess(BYTE *pData, UINT32 numFrames, WAVEFORMATEX *pwfx) {
    UINT32 numSamples = numFrames * pwfx->nChannels;

    switch (pwfx->wBitsPerSample) {
        case 16:
            for (UINT32 i = 0; i < numSamples; ++i) {
                std::swap(pData[i * 2], pData[i * 2 + 1]);
            }
            break;
        case 24:
            for (UINT32 i = 0; i < numSamples; ++i) {
                std::swap(pData[i * 3], pData[i * 3 + 2]);
            }
            break;
        case 32:
            for (UINT32 i = 0; i < numSamples; ++i) {
                std::swap(pData[i * 4], pData[i * 4 + 3]);
                std::swap(pData[i * 4 + 1], pData[i * 4 + 2]);
            }
            break;
    }

    return OK;
}

HRESULT init_client(IAudioClient *pAudioClient, WAVEFORMATEX *format, int secs_in_buffer) {
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

int init_capturer(IMMDeviceEnumerator *&enumerator, IMMDevice *&recorder, IAudioClient *&pAudioClient, WAVEFORMATEX *&format) {
    HRESULT hr = OK;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HANDLE_ERROR(hr, "CoInitializeEx", done);

    hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        IID_IMMDeviceEnumerator,
        (void**)&enumerator
    );
    HANDLE_ERROR(hr, "CoCreateInstance", done);

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &recorder);
    HANDLE_ERROR(hr, "GetDefaultAudioEndpoint", done);

    hr = recorder->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    HANDLE_ERROR(hr, "Activate", done);

    hr = pAudioClient->GetMixFormat(&format);
    HANDLE_ERROR(hr, "GetMixFormat", done);

    print_format(format);

    hr = init_client(pAudioClient, format, SECONDS_IN_SHARED_BUFFER);
    HANDLE_ERROR(hr, "init_client", done);

done:
    return hr;
}

HRESULT capture_sound(IAudioClient *pAudioClient, IAudioCaptureClient *&pCaptureClient, WAVEFORMATEX *format, int init_data(WAVEFORMATEX *format), int (*proc_data)(BYTE *captureBuffer, UINT32 bytes_captured, WAVEFORMATEX *format), int (*finish)(void), bool (*enough)(UINT64 bytes_in_second)) {
    HRESULT rc_capture = OK;
    int rc_process = OK;

    bool RUNNING = true;

    UINT32 nFrames;
    DWORD flags;
    BYTE *captureBuffer;
    UINT32 packetLength = 0;

    UINT64 bytes_in_second;

    rc_capture = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    HANDLE_ERROR(rc_capture, "GetService", loop_end);
    rc_capture = pAudioClient->Start();
    HANDLE_ERROR(rc_capture, "Start", loop_end);

    rc_process = init_data(format);
    if (rc_process != OK) {
        cout << "init_data: something went wrong (" << rc_process << ")" << endl;
        return rc_process;
    }

    bytes_in_second = format->nSamplesPerSec * (format->wBitsPerSample / 8) * format->nChannels;

    while (RUNNING && !enough(bytes_in_second)) {
        pCaptureClient->GetNextPacketSize(&packetLength);
        HANDLE_ERROR(rc_capture, "GetNextPacketSize", loop_end);
    
        while (RUNNING && packetLength != 0 && !enough(bytes_in_second)) {
            rc_capture = pCaptureClient->GetBuffer(&captureBuffer, &nFrames, &flags, NULL, NULL);
            HANDLE_ERROR(rc_capture, "GetBuffer", loop_end);

            if (nFrames != 0) {
                rc_process = proc_data(captureBuffer, nFrames, format);
                if (rc_process != OK)
                    RUNNING = false;
            }

            rc_capture = pCaptureClient->ReleaseBuffer(nFrames);
            HANDLE_ERROR(rc_capture, "ReleaseBuffer", loop_end);
            
            pCaptureClient->GetNextPacketSize(&packetLength);
            HANDLE_ERROR(rc_capture, "GetNextPacketSize", loop_end);
        }
    }

loop_end:
    finish();
    pAudioClient->Stop();

    if (rc_capture == OK)
        return rc_process;

    return rc_capture;
}
