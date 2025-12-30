#pragma once

#include "exceptions/BaseException.hpp"

class BaseRecorderException : public BaseException {
public:
    BaseRecorderException(
        const char *filename,
        const char *funcName,
        const int line,
        const char *text = "Base Recorder Exception"
    ): BaseException(filename, funcName, line, text) {}
};
