#pragma once

#include <cstdio>
#include <iostream>
#include <windows.h>

#include <mmreg.h>

#include "ErrorCodes.hpp"
#include "WaveConstants.hpp"
#include "consumer/IConsumerService.hpp"

using std::cout;
using std::endl;

class WaveCreator final : public IConsumerService {
public:
    explicit WaveCreator(const char *filename = WaveConstants::DEFAULT_FILENAME);

    errcode_t initialize(WAVEFORMATEX *format) override;

    // Adds new data in bytes to the initialized wave file
    errcode_t consumeNewData(BYTE *data, UINT32 bytesCount) override;

    bool isEnough(UINT64 bytesInSecond, double secondsNeed);

    void destroy() override;

private:
    const char *filename = nullptr;

    FILE *file = nullptr;
    UINT64 totalSizeWritten = 0;

    WAVEFORMATEX *format{};

    DWORD cksize = sizeof(WAVEFORMATEXTENSIBLE);
    long long HEADER_SIZE = 4 + 4 + 4 + 4 + 4 + cksize + 4 + 4; // 68

    void closeFile();
};
