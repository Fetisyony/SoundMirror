#include <iostream>

#include "ErrorCodes.hpp"
#include "spdlog/spdlog.h"

void showWasapiErrorMessage(HRESULT er) {
    switch (er) {
        case OK:
            break;
        case (AUDCLNT_E_ALREADY_INITIALIZED):
            spdlog::error("AUDCLNT_E_ALREADY_INITIALIZED");
            break;
        case (AUDCLNT_E_WRONG_ENDPOINT_TYPE):
            spdlog::error("AUDCLNT_E_WRONG_ENDPOINT_TYPE");
            break;
        case (AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED):
            spdlog::error("AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED");
            break;
        case (AUDCLNT_E_BUFFER_SIZE_ERROR):
            spdlog::error("AUDCLNT_E_BUFFER_SIZE_ERROR");
            break;
        case (AUDCLNT_E_CPUUSAGE_EXCEEDED):
            spdlog::error("AUDCLNT_E_CPUUSAGE_EXCEEDED");
            break;
        case (AUDCLNT_E_DEVICE_INVALIDATED):
            spdlog::error("AUDCLNT_E_DEVICE_INVALIDATED");
            break;
        case (AUDCLNT_E_DEVICE_IN_USE):
            spdlog::error("AUDCLNT_E_DEVICE_IN_USE");
            break;
        case (AUDCLNT_E_ENDPOINT_CREATE_FAILED):
            spdlog::error("AUDCLNT_E_ENDPOINT_CREATE_FAILED");
            break;
        case (AUDCLNT_E_INVALID_DEVICE_PERIOD):
            spdlog::error("AUDCLNT_E_INVALID_DEVICE_PERIOD");
            break;
        case (AUDCLNT_E_UNSUPPORTED_FORMAT):
            spdlog::error("AUDCLNT_E_UNSUPPORTED_FORMAT");
            break;
        case (AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED):
            spdlog::error("AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED");
            break;
        case (AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL):
            spdlog::error("AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL");
            break;
        case (AUDCLNT_E_SERVICE_NOT_RUNNING):
            spdlog::error("AUDCLNT_E_SERVICE_NOT_RUNNING");
            break;
        case (E_POINTER):
            spdlog::error("E_POINTER");
            break;
        case (E_INVALIDARG):
            spdlog::error("E_INVALIDARG");
            break;
        case (E_OUTOFMEMORY):
            spdlog::error("E_OUTOFMEMORY");
            break;
        case (E_NOINTERFACE):

            spdlog::error("E_NOINTERFACE");
            break;
        case (E_NOTIMPL):
            spdlog::error("E_NOTIMPL");
            break;
        case (E_FAIL):
            spdlog::error("E_FAIL");
            break;
        case (E_ACCESSDENIED):
            spdlog::error("E_ACCESSDENIED");
            break;
        case (AUDCLNT_E_RESOURCES_INVALIDATED):
            spdlog::error("AUDCLNT_E_RESOURCES_INVALIDATED");
            break;
        default:
            spdlog::error("Unknown error");
            break;
    }
}
