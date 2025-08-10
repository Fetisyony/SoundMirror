#pragma once

#include <exception>
#include <cstdio>
#include <ctime>

class BaseException : public std::exception {
public:
    BaseException(const char *filename,
                  const char *funcName,
                  const int line,
                  const char *text) {
        const time_t now = time(nullptr);
        snprintf(errorText, maxErrorTextLen, format, filename, funcName, line, ctime(&now), text);
    }

    [[nodiscard]] const char *what() const noexcept override {
        return errorText;
    }

    ~BaseException() override = default;

protected:
    static constexpr int maxErrorTextLen = 512;

    char errorText[maxErrorTextLen]{};

private:
    static constexpr auto format = "[Exception] Filename: %s\nFunction: %s\nLine: %d\nTime: %s\nError: %s";
};
