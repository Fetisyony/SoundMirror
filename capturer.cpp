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
    if (pwfx->wBitsPerSample == 16) {
        UINT32 numSamples = numFrames * pwfx->nChannels;
        for (UINT32 i = 0; i < numSamples; ++i) {
            BYTE temp = pData[i * 2];
            pData[i * 2] = pData[i * 2 + 1];
            pData[i * 2 + 1] = temp;
        }
    } else if (pwfx->wBitsPerSample == 24) {
        UINT32 numSamples = numFrames * pwfx->nChannels;
        for (UINT32 i = 0; i < numSamples; ++i) {
            BYTE temp = pData[i * 3];
            pData[i * 3] = pData[i * 3 + 2];
            pData[i * 3 + 2] = temp;
        }
    } else if (pwfx->wBitsPerSample == 32) {
        UINT32 numSamples = numFrames * pwfx->nChannels;
        for (UINT32 i = 0; i < numSamples; ++i) {
            BYTE temp1 = pData[i * 4];
            BYTE temp2 = pData[i * 4 + 1];
            pData[i * 4] = pData[i * 4 + 3];
            pData[i * 4 + 1] = pData[i * 4 + 2];
            pData[i * 4 + 2] = temp2;
            pData[i * 4 + 3] = temp1;
        }
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
    HRESULT hr = OK;

    bool RUNNING = true;

    UINT32 nFrames;
    DWORD flags;
    BYTE *captureBuffer;
    UINT32 packetLength = 0;

    UINT64 bytes_in_second;

    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    HANDLE_ERROR(hr, "GetService", loop_end);
    hr = pAudioClient->Start();
    HANDLE_ERROR(hr, "Start", loop_end);

    hr = init_data(format);
    if (hr != OK) {
        cout << "init_data: something went wrong: " << hr << endl;
        return hr;
    }

    bytes_in_second = format->nSamplesPerSec * (format->wBitsPerSample / 8) * format->nChannels;

    while (RUNNING && !enough(bytes_in_second)) {
        pCaptureClient->GetNextPacketSize(&packetLength);
        HANDLE_ERROR(hr, "GetNextPacketSize", loop_end);
    
        while (RUNNING && packetLength != 0 && !enough(bytes_in_second)) {
            hr = pCaptureClient->GetBuffer(&captureBuffer, &nFrames, &flags, NULL, NULL);
            HANDLE_ERROR(hr, "GetBuffer", loop_end);

            if (nFrames != 0) {
                hr = proc_data(captureBuffer, nFrames, format);
                if (hr != OK)
                    RUNNING = false;
            }

            hr = pCaptureClient->ReleaseBuffer(nFrames);
            HANDLE_ERROR(hr, "ReleaseBuffer", loop_end);
            
            pCaptureClient->GetNextPacketSize(&packetLength);
            HANDLE_ERROR(hr, "GetNextPacketSize", loop_end);
        }
    }

loop_end:
    finish();
    pAudioClient->Stop();

    return hr;
}
