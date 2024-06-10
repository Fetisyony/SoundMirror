#include "capturer.h"

const char *filename = "output.wav";

DWORD cksize = sizeof(WAVEFORMATEXTENSIBLE);
int HEADER_SIZE = 4 + 4 + 4 + 4 + 4 + cksize + 4 + 4;  // 68


#define HANDLE_ERROR(hr, message, label) if (FAILED(hr)) { \
    std::cout << "Error: " << message << "- failed (" << hr << ")" << std::endl; \
    goto label; \
}


int main() {
    int hr = OK;

    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *recorder = NULL;
    IAudioClient *pAudioClient = NULL;
    IAudioCaptureClient *pCaptureClient = NULL;
    WAVEFORMATEX *format = NULL;

    hr = init_capturer(enumerator, recorder, pAudioClient, format);

    if (SUCCEEDED(hr)) {
        hr = run_service(pAudioClient, pCaptureClient, format);
        HANDLE_ERROR(hr, "run_service", done);
    }


done:
    SAFE_RELEASE(enumerator)
    CoTaskMemFree(format);
    SAFE_RELEASE(pCaptureClient)
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(recorder)

    CoUninitialize();

    return hr;
}

HRESULT check_error(HRESULT hres, std::string message) {
    if (FAILED(hres)) {
        std::cout << "WTF: " << message << "- failed (" << hres << ")" << std::endl;
    }
    return hres;
}

void print_format(WAVEFORMATEX *format) {
    printf("  Frame size     : %d\n", format->nBlockAlign);
    printf("  Channels       : %d\n", format->nChannels);
    printf("  Bits per second: %d\n", format->wBitsPerSample);
    printf("  Sample rate:   : %ld\n", format->nSamplesPerSec);
    printf("  Format:   : %d\n", format->wFormatTag);
    printf("  Size:   : %d\n", format->cbSize);
}

void initWavFile(FILE *file, WAVEFORMATEX *format) {
    WAVEFORMATEXTENSIBLE *pWaveFormatExtensible;
    pWaveFormatExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(format);

    assert (pWaveFormatExtensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    int file_size = HEADER_SIZE;

    fwrite("RIFF", 1, 4, file);         // offset 0
    fwrite(&file_size, 4, 1, file);     // offset 4

    fwrite("WAVE", 1, 4, file);         // offset 8
    fwrite("fmt ", 1, 4, file);         // offset 12
    fwrite(&cksize, 4, 1, file);        // offset 16
    fwrite(format, cksize, 1, file);    // offset 20

    fwrite("data", 1, 4, file);         // offset 60
    // fwrite(&dataSize, 4, 1, file);   // offset 64
    // fwrite(data, dataSize, 1, file); // offset 68
}

void writeWavData(FILE *file, BYTE *data, int oldDataSize, int newDataSize) {
    int fullDataSize = newDataSize + oldDataSize;
    int file_size = HEADER_SIZE + fullDataSize;

    fseek(file, 4, SEEK_SET);
    fwrite(&file_size, 4, 1, file);

    fseek(file, 64, SEEK_SET);
    fwrite(&fullDataSize, 4, 1, file);

    fseek(file, HEADER_SIZE + oldDataSize, SEEK_SET);
    fwrite(data, newDataSize, 1, file);
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

    UINT32 tmp;
    pAudioClient->GetBufferSize(&tmp);
    std::cout << "Buffer frames number: " << tmp << std::endl;
    std::cout << "Buffer size: " << ((10000.0 * 1000 / format->nSamplesPerSec * tmp) + 0.5) << std::endl;
    std::cout << "Requested buffer size: " << hnsBufferDuration << std::endl;

    return hr;
}

HRESULT run_service(IAudioClient *pAudioClient, IAudioCaptureClient *&pCaptureClient, WAVEFORMATEX *format) {
    HRESULT hr = OK;

    UINT64 totalBytesWritten = 0;
    UINT32 nFrames;
    DWORD flags;
    BYTE *captureBuffer;

    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    assert(SUCCEEDED(hr));
    hr = pAudioClient->Start();
    assert(SUCCEEDED(hr));

    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error opening file\n");
        return FILE_ERROR;
    }

    initWavFile(file, format);

    UINT64 bytes_in_second = format->nSamplesPerSec * (format->wBitsPerSample / 8) * format->nChannels;
    while (totalBytesWritten < bytes_in_second * RECORDING_DURATION_SECONDS) {
        hr = pCaptureClient->GetBuffer(&captureBuffer, &nFrames, &flags, NULL, NULL);
        assert(SUCCEEDED(hr));

        if (nFrames != 0) {
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                puts("There is silence!");  // Tell CopyData to write silence.
            }

            UINT32 bytes_captured = format->nBlockAlign * nFrames;

            writeWavData(file, captureBuffer, totalBytesWritten, bytes_captured);

            totalBytesWritten += bytes_captured;
        }

        hr = pCaptureClient->ReleaseBuffer(nFrames);
        if (FAILED(hr)) {
            std::cout << "ReleaseBuffer failed (" << hr << ")" << std::endl;
            break;
        }
    }

    fclose(file);
    pAudioClient->Stop();

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
