#pragma once
#include "BaseSocketServerException.hpp"

class NotImplementedException final : public BaseSocketServerException {
public:
    NotImplementedException(
        const char *filename,
        const char *funcName,
        const int line,
        const char *text = "Method not implemented"
    ) : BaseSocketServerException(filename, funcName, line, text) {}
};
