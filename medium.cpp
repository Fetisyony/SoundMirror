#include <stdio.h>
#include <Windows.h>
#include <initguid.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <assert.h>
#include <mmreg.h>
#include <iostream>

// Define the length of audio capture in seconds
#define RECORDING_DURATION_SECONDS 10

void createWavFile(FILE *file, WAVEFORMATEX *format, BYTE *data, int dataSize) {
    WAVEFORMATEXTENSIBLE *pWaveFormatExtensible;
    switch(format->wFormatTag)
    {
        case WAVE_FORMAT_PCM:
            std::cout << "WAVE_FORMAT_PCM" << std::endl;
            break;

        case WAVE_FORMAT_IEEE_FLOAT:
            std::cout << "WAVE_FORMAT_IEEE_FLOAT" << std::endl;
            break;

        case WAVE_FORMAT_EXTENSIBLE:
            std::cout << "WAVE_FORMAT_EXTENSIBLE" << std::endl;

            pWaveFormatExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(format);

            if(pWaveFormatExtensible->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
            {
                std::cout << "KSDATAFORMAT_SUBTYPE_PCM" << std::endl;
            }
            else if(pWaveFormatExtensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
            {
                std::cout << "KSDATAFORMAT_SUBTYPE_IEEE_FLOAT" << std::endl;
            }
            break;
    }

    int file_size = 68 + dataSize;
    int dwSampleLength = dataSize / format->nBlockAlign;
    DWORD cksize = 40;

    fwrite("RIFF", 1, 4, file);                   // offset 0
    fwrite(&file_size, 4, 1, file);               // offset 4

    fwrite("WAVE", 1, 4, file);                   // offset 8
    fwrite("fmt ", 1, 4, file);                   // offset 12
    fwrite(&cksize, 4, 1, file);                  // offset 16
    fwrite(format, sizeof(WAVEFORMATEXTENSIBLE), 1, file);      // offset 20 (0x14)

    fwrite("fact", 1, 4, file);                   // offset 60
    fwrite(&dwSampleLength, 4, 1, file);          // offset 64

    fwrite("data", 1, 4, file);                   // offset 68
    fwrite(&dataSize, 4, 1, file);                // offset 72
    fwrite(data, dataSize, 1, file);              // offset 76
}

// writing data to file
void writeWavData(FILE *file, BYTE *data, UINT32 numFrames, int oldDataSize, WORD nBlockAlign) {
    int newDataSize = numFrames * nBlockAlign;
    int fullDataSize = newDataSize + oldDataSize;
    int file_size = 68 + fullDataSize;
    int dwSampleLength = fullDataSize / nBlockAlign;

    fseek(file, 4, SEEK_SET);
    fwrite(&file_size, 4, 1, file);

    fseek(file, 64, SEEK_SET);
    fwrite(&dwSampleLength, 4, 1, file);

    fseek(file, 72, SEEK_SET);
    fwrite(&fullDataSize, 4, 1, file);

    fseek(file, 76 + oldDataSize, SEEK_SET);
    fwrite(data, newDataSize, 1, file);
}


int main() {
    const char *filename = "output.wav";

    HRESULT hr;
    IMMDeviceEnumerator* enumerator = NULL;
    IMMDevice* recorder = NULL;
    IAudioClient* recorderClient = NULL;
    IAudioCaptureClient* captureService = NULL;
    WAVEFORMATEX* format = NULL;

    // Open the WAV file for writing
    UINT64 totalBytesWritten = 0;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        IID_IMMDeviceEnumerator,
        (void**)&enumerator
    );
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &recorder);
    hr = enumerator->Release();
    hr = recorder->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&recorderClient);
    hr = recorderClient->GetMixFormat(&format);

    std::cout << "Mix format:\n" << std::endl;
    printf("  Frame size     : %d\n", format->nBlockAlign);
    printf("  Channels       : %d\n", format->nChannels);
    printf("  Bits per second: %d\n", format->wBitsPerSample);
    printf("  Sample rate:   : %d\n", format->nSamplesPerSec);
    printf("  Format:   : %d\n", format->wFormatTag);
    printf("  Size:   : %d\n", format->cbSize);
    printf("wFormatTag: %d\n", format->wFormatTag);

    hr = recorderClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 10000000, 0, format, NULL);
    assert(SUCCEEDED(hr));

    hr = recorderClient->GetService(IID_IAudioCaptureClient, (void**)&captureService);
    assert(SUCCEEDED(hr));

    UINT32 nFrames;
    DWORD flags;
    BYTE* captureBuffer;

    hr = recorderClient->Start();
    assert(SUCCEEDED(hr));

    bool first = true;
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    while (totalBytesWritten < 2000000) {
        hr = captureService->GetBuffer(&captureBuffer, &nFrames, &flags, NULL, NULL);
        assert(SUCCEEDED(hr));

        // write to file
        if (format->nBlockAlign * nFrames) {
            if (first) {
                createWavFile(file, format, captureBuffer, format->nBlockAlign * nFrames);
                first = false;
            } else {
                writeWavData(file, captureBuffer, nFrames, totalBytesWritten, format->nBlockAlign);
                printf("Bytes written: %d\n", format->nBlockAlign * nFrames);
            }

            totalBytesWritten += format->nBlockAlign * nFrames;
        }
        hr = captureService->ReleaseBuffer(nFrames);
        assert(SUCCEEDED(hr));
    }

    fclose(file);
    // ... (Cleanup code remains the same)

    // This code won't be reached but if the loop condition changes
    // you should always clear the resources

    recorderClient->Stop();

    captureService->Release();

    recorderClient->Release();

    recorder->Release();

    CoUninitialize();
}