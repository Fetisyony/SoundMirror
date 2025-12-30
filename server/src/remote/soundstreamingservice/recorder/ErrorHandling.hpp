#pragma once

#include "spdlog/spdlog.h"

#define HANDLE_RET_CODE(hr, message, label) if (FAILED(hr)) { \
    spdlog::error("Error: {} - failed ({})", message, (long)hr); \
    goto label; \
}
