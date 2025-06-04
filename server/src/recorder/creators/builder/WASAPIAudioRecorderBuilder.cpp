#define INITGUID
#include "WASAPIAudioRecorderBuilder.hpp"
#include <ksmedia.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>
#include <mmdeviceapi.h>
#include <combaseapi.h>
#include "ErrorCodes.hpp"
#include "recorder/utils.hpp"
#include "recorder/WASAPIAudioRecorder.hpp"

HRESULT WASAPIAudioRecorderBuilder::initEnumerator() {
    HRESULT hr = OK;

    // Initializes the COM library for use by the calling thread,
    // sets the thread's concurrency model, and creates a new apartment for the thread if one is required.
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    HANDLE_RET_CODE(hr, "CoInitializeEx", done);

    hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator,
        nullptr,
        CLSCTX_ALL,
        IID_IMMDeviceEnumerator,
        reinterpret_cast<void **>(&_recorder->enumerator)
    );
    HANDLE_RET_CODE(hr, "CoCreateInstance", done);

done:
    return hr;
}

HRESULT WASAPIAudioRecorderBuilder::activateClient() {
    HRESULT hr = OK;

    hr = _recorder->recorder->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr,
                                       reinterpret_cast<void **>(&_recorder->pAudioClient));
    HANDLE_RET_CODE(hr, "Activate", done);

done:
    return hr;
}

HRESULT WASAPIAudioRecorderBuilder::initializeLoopbackRecorder() {
    HRESULT hr = OK;

    hr = _recorder->enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &_recorder->recorder);
    HANDLE_RET_CODE(hr, "GetDefaultAudioEndpoint", done);

done:
    return hr;
}

HRESULT WASAPIAudioRecorderBuilder::initializeMicrophoneRecorder() {
    HRESULT hr = OK;

    hr = _recorder->enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &_recorder->recorder);
    HANDLE_RET_CODE(hr, "GetDefaultAudioEndpoint", done);

done:
    return hr;
}

HRESULT WASAPIAudioRecorderBuilder::initializeSharedModeFormat() {
    HRESULT hr = OK;

    hr = _recorder->pAudioClient->GetMixFormat(&_recorder->format);
    HANDLE_RET_CODE(hr, "GetMixFormat", done);

done:
    return hr;
}

HRESULT WASAPIAudioRecorderBuilder::initializeExclusiveModeFormat() {
    HRESULT hr = OK;

    IPropertyStore *pProps = nullptr;
    PROPVARIANT varFormat;
    WAVEFORMATEX *pwfx;

    hr = _recorder->recorder->OpenPropertyStore(STGM_READ, &pProps);
    HANDLE_RET_CODE(hr, "OpenPropertyStore", done);

    PropVariantInit(&varFormat);
    hr = pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &varFormat);
    HANDLE_RET_CODE(hr, "GetValue", done);

    pwfx = reinterpret_cast<WAVEFORMATEX *>(varFormat.blob.pBlobData);

    hr = _recorder->pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, pwfx, nullptr);
    HANDLE_RET_CODE(hr, "IsFormatSupported", done);

    _recorder->format = pwfx;

done:
    return hr;
}

HRESULT WASAPIAudioRecorderBuilder::setEventHandle(HANDLE captureEvent) {
    HRESULT hr = OK;

    hr = _recorder->pAudioClient->SetEventHandle(captureEvent);
    HANDLE_RET_CODE(hr, "SetEventHandle", done);

done:
    return hr;
}

HRESULT WASAPIAudioRecorderBuilder::initializeExclusiveClient(int secs_in_buffer) {
    HRESULT hr = OK;

    (void)secs_in_buffer;

    REFERENCE_TIME hnsDefaultPeriod, hnsMinPeriod;
    hr = _recorder->pAudioClient->GetDevicePeriod(&hnsDefaultPeriod, &hnsMinPeriod);
    HANDLE_RET_CODE(hr, "GetDevicePeriod", done);

    hr = _recorder->pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_EXCLUSIVE,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        hnsDefaultPeriod,
        hnsDefaultPeriod, // hnsPeriodicity
        _recorder->format,
        nullptr
    );
    HANDLE_RET_CODE(hr, "Initialize", done);

done:
    return hr;
}

HRESULT WASAPIAudioRecorderBuilder::initializeSharedClient(int secs_in_buffer) {
    HRESULT hr = OK;

    // hecto nanoseconds
    REFERENCE_TIME hnsBufferDuration = 10 * 1000 * 1000 * secs_in_buffer;
    hr = _recorder->pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_LOOPBACK, // | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
        hnsBufferDuration,
        0, // hnsPeriodicity is always 0 in shared mode
        _recorder->format,
        nullptr
    );
    HANDLE_RET_CODE(hr, "Initialize", done);

done:
    return hr;
}

HRESULT WASAPIAudioRecorderBuilder::getService() {
    HRESULT hr = OK;

    hr = _recorder->pAudioClient->GetService(IID_IAudioCaptureClient,
                                             reinterpret_cast<void **>(&_recorder->pCaptureClient));
    HANDLE_RET_CODE(hr, "GetService", done);

done:
    return hr;
}

void WASAPIAudioRecorderBuilder::listAudioCaptureDevices() {
    HRESULT hr = OK;

    Microsoft::WRL::ComPtr<IMMDeviceCollection> collection;
    hr = _recorder->enumerator->EnumAudioEndpoints(
        eCapture,
        DEVICE_STATE_ACTIVE,
        collection.GetAddressOf()
    );

    if (FAILED(hr)) {
        std::cerr << "Device enumeration failed: 0x" << std::hex << hr << std::endl;
        CoUninitialize();
        return;
    }

    UINT count = 0;
    hr = collection->GetCount(&count);
    if (FAILED(hr) || count == 0) {
        std::cout << "No audio capture devices found" << std::endl;
        CoUninitialize();
        return;
    }

    std::cout << "\nAvailable Capture Devices (" << count << "):\n";
    std::cout << "---------------------------------\n";

    for (UINT i = 0; i < count; ++i) {
        Microsoft::WRL::ComPtr<IMMDevice> device;
        hr = collection->Item(i, device.GetAddressOf());
        if (FAILED(hr)) continue;

        // Get device ID
        LPWSTR deviceId = nullptr;
        if (SUCCEEDED(device->GetId(&deviceId))) {
            std::wcout << L"[" << i << L"] ID: " << deviceId << L"\n";
            CoTaskMemFree(deviceId);
        }

        // Get friendly name
        Microsoft::WRL::ComPtr<IPropertyStore> props;
        hr = device->OpenPropertyStore(STGM_READ, props.GetAddressOf());
        if (SUCCEEDED(hr)) {
            PROPVARIANT varName;
            PropVariantInit(&varName);

            if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &varName))) {
                if (varName.vt == VT_LPWSTR && varName.pwszVal != nullptr) {
                    std::wcout << L"    Name: " << varName.pwszVal << L"\n";
                }
            }
            PropVariantClear(&varName);
        }
        std::cout << "---------------------------------\n";
    }
}

std::shared_ptr<WASAPIAudioRecorder> WASAPIAudioRecorderBuilder::build() {
    return _recorder;
}
