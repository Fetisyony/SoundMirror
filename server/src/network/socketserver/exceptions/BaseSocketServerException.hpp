#pragma once

#include "exceptions/BaseException.hpp"

class BaseSocketServerException : public BaseException {
public:
    BaseSocketServerException(
        const char *filename,
        const char *funcName,
        const int line,
        const char *text = "Base Socket Server Exception"
    ): BaseException(filename, funcName, line, text) {}
};
