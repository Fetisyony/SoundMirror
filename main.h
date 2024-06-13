#pragma once

#include "capturer.h"
#include "server.h"
#include "server_errors.h"

int launch_streaming();

int convert_endianess_and_send(BYTE* pData, UINT32 nFrames, WAVEFORMATEX *format);

int run_socket_recording(IMMDeviceEnumerator *enumerator, IMMDevice *recorder, IAudioClient *pAudioClient, IAudioCaptureClient *pCaptureClient, WAVEFORMATEX *format);

int run_wav_recording(IMMDeviceEnumerator *enumerator, IMMDevice *recorder, IAudioClient *pAudioClient, IAudioCaptureClient *pCaptureClient, WAVEFORMATEX *format);
