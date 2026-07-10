#pragma once
#include <windows.h>

class IResampler {
public:
    virtual ~IResampler() = default;

    virtual bool process(const BYTE* inBuffer, UINT64 inBytes, BYTE* outBuffer, UINT64& outBytes, UINT64 outCapacity) = 0;
};
