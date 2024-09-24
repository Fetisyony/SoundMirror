#pragma once

#include "errors.h"


void show_error_init_client(HRESULT er) {
    switch (er) {
        case (AUDCLNT_E_ALREADY_INITIALIZED):
            cout << "AUDCLNT_E_ALREADY_INITIALIZED" << endl;
            break;
        case (AUDCLNT_E_WRONG_ENDPOINT_TYPE):
            cout << "AUDCLNT_E_WRONG_ENDPOINT_TYPE" << endl;
            break;
        case (AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED):
            cout << "AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED" << endl;
            break;
        case (AUDCLNT_E_BUFFER_SIZE_ERROR):
            cout << "AUDCLNT_E_BUFFER_SIZE_ERROR" << endl;
            break;
        case (AUDCLNT_E_CPUUSAGE_EXCEEDED):
            cout << "AUDCLNT_E_CPUUSAGE_EXCEEDED" << endl;
            break;
        case (AUDCLNT_E_DEVICE_INVALIDATED):
            cout << "AUDCLNT_E_DEVICE_INVALIDATED" << endl;
            break;
        case (AUDCLNT_E_DEVICE_IN_USE):
            cout << "AUDCLNT_E_DEVICE_IN_USE" << endl;
            break;
        case (AUDCLNT_E_ENDPOINT_CREATE_FAILED):
            cout << "AUDCLNT_E_ENDPOINT_CREATE_FAILED" << endl;
            break;
        case (AUDCLNT_E_INVALID_DEVICE_PERIOD):
            cout << "AUDCLNT_E_INVALID_DEVICE_PERIOD" << endl;
            break;
        case (AUDCLNT_E_UNSUPPORTED_FORMAT):
            cout << "AUDCLNT_E_UNSUPPORTED_FORMAT" << endl;
            break;
        case (AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED):
            cout << "AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED" << endl;
            break;
        case (AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL):
            cout << "AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL" << endl;
            break;
        case (AUDCLNT_E_SERVICE_NOT_RUNNING):
            cout << "AUDCLNT_E_SERVICE_NOT_RUNNING" << endl;
            break;
        case (E_POINTER):
            cout << "E_POINTER" << endl;
            break;
        case (E_INVALIDARG):
            cout << "E_INVALIDARG" << endl;
            break;
        case (E_OUTOFMEMORY):
            cout << "E_OUTOFMEMORY" << endl;
            break;
        default:
            cout << "Unknown error" << endl;
            break;
    }
}
