#include "utils.hpp"
#include <iostream>

#include "spdlog/spdlog.h"

void printFormat(const WAVEFORMATEX *format) {
    spdlog::info("Frame size     : {}", format->nBlockAlign);
    spdlog::info("Channels       : {}", format->nChannels);
    spdlog::info("Bits per second: {}", format->wBitsPerSample);
    spdlog::info("Sample rate    : {}", format->nSamplesPerSec);
    spdlog::info("Format         : {}", format->wFormatTag);
    spdlog::info("Size           : {}", format->cbSize);
}
