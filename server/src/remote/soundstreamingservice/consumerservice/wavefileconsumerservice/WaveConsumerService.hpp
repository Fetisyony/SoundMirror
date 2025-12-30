#pragma once

#include <cstdio>
#include <iostream>
#include <windows.h>

#include <mmreg.h>

#include "ErrorCodes.hpp"
#include "wave_constants.hpp"
#include "remote/soundstreamingservice/consumerservice/IConsumerService.hpp"

class WaveConsumerService final : public IConsumerService {
public:
    explicit WaveConsumerService(const char *filename = wave_constants::DEFAULT_FILENAME);

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
