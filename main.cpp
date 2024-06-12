#include "main.h"


SOCKET clientSocket;
SOCKET listenSocket;

UINT64 passed = 0;


int main() {
    int hr = OK;

    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *recorder = NULL;
    IAudioClient *pAudioClient = NULL;
    IAudioCaptureClient *pCaptureClient = NULL;
    WAVEFORMATEX *format = NULL;

    hr = run_socket_recording(enumerator, recorder, pAudioClient, pCaptureClient, format);
    HANDLE_ERROR(hr, "run_socket_recording", exit);

exit:
    SAFE_RELEASE(enumerator)
    CoTaskMemFree(format);
    SAFE_RELEASE(pCaptureClient)
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(recorder)

    CoUninitialize();

    return hr;
}

int convert_endianess_and_send(BYTE* pData, UINT32 nFrames, WAVEFORMATEX *format) {
    int rc = OK;
    UINT32 bytes_captured = format->nBlockAlign * nFrames;
    // Convert the audio data from little-endian to big-endian
    swap_sound_endianess(pData, nFrames, format);

    // Here you can save the data, send it to another process, etc.
    rc = send_to_client(pData, bytes_captured);
    return rc;
}

int run_socket_recording(IMMDeviceEnumerator *enumerator, IMMDevice *recorder, IAudioClient *pAudioClient, IAudioCaptureClient *pCaptureClient, WAVEFORMATEX *format) {
    int hr = OK;

    hr = init_server(listenSocket);
    cout << "Server inited with code " << hr << endl;

    if (hr == OK) {
        hr = init_capturer(enumerator, recorder, pAudioClient, format);
        HANDLE_ERROR(hr, "init_capturer", exit);

        hr = capture_sound(pAudioClient, pCaptureClient, format, announce_format, convert_endianess_and_send, close_socket, get_false);
        HANDLE_ERROR(hr, "capture_to_file", exit);

        cout << "Server sended with code " << hr << endl;
    }

exit:
    (void)closesocket(clientSocket);
    (void)closesocket(listenSocket);
    WSACleanup();

    return hr;
}

int run_wav_recording(IMMDeviceEnumerator *enumerator, IMMDevice *recorder, IAudioClient *pAudioClient, IAudioCaptureClient *pCaptureClient, WAVEFORMATEX *format) {
    int hr = OK;

    hr = init_capturer(enumerator, recorder, pAudioClient, format);
    HANDLE_ERROR(hr, "init_capturer", done);

    hr = capture_sound(pAudioClient, pCaptureClient, format, initWavFile, writeWavData, close_file, is_enough);
    HANDLE_ERROR(hr, "capture_to_file", done);

done:
    return hr;
}