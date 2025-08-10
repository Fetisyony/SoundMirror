#pragma once
#include "BaseRecorderException.hpp"

class RecorderCreationFailed final : public BaseRecorderException {
public:
    RecorderCreationFailed(
        const char *filename,
        const char *funcName,
        const int line,
        const char *text = "Failed to create a recorder"
    ) : BaseRecorderException(filename, funcName, line, text) {}
};