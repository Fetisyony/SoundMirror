#include "utils.hpp"

void printFormat(const WAVEFORMATEX *format) {
    printf("Frame size     : %d\n", format->nBlockAlign);
    printf("Channels       : %d\n", format->nChannels);
    printf("Bits per second: %d\n", format->wBitsPerSample);
    printf("Sample rate:   : %ld\n", format->nSamplesPerSec);
    printf("Format:        : %d\n", format->wFormatTag);
    printf("Size:          : %d\n", format->cbSize);
}
