#include "WaveCreator.hpp"

#include <cassert>


WaveCreator::WaveCreator(const char *filename) : filename(filename) {}

errcode_t WaveCreator::initialize(WAVEFORMATEX *format) {
    this->format = format;

    file = fopen(filename, "wb");
    if (!file) {
        throw std::runtime_error("Unable to initialize wave file!");
    }

    // WAVEFORMATEXTENSIBLE *pWaveFormatExtensible;
    // pWaveFormatExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(format);
    // assert(pWaveFormatExtensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    int file_size = HEADER_SIZE;

    fwrite("RIFF", 1, 4, file);         // offset 0
    fwrite(&file_size, 4, 1, file);     // offset 4

    fwrite("WAVE", 1, 4, file);         // offset 8
    fwrite("fmt ", 1, 4, file);         // offset 12
    fwrite(&cksize, 4, 1, file);        // offset 16
    fwrite(format, cksize, 1, file);    // offset 20

    fwrite("data", 1, 4, file);         // offset 60

    // fwrite(&dataSize, 4, 1, file);   // offset 64
    // fwrite(data, dataSize, 1, file); // offset 68

    fflush(file);
    return OK;
}

// Adds new data in bytes to the initialized wave file
errcode_t WaveCreator::consumeNewData(BYTE *data, UINT32 bytesCount) {
    if (!file) {
        throw std::runtime_error("Unable to consumeNewData to uninitialized wave file!");
    }

    int newTotalSizeWritten = bytesCount + totalSizeWritten;
    int file_size = HEADER_SIZE + newTotalSizeWritten;

    fseek(file, 4, SEEK_SET);
    fwrite(&file_size, 4, 1, file);

    fseek(file, 64, SEEK_SET);
    fwrite(&newTotalSizeWritten, 4, 1, file);

    fseek(file, HEADER_SIZE + totalSizeWritten, SEEK_SET);
    fwrite(data, bytesCount, 1, file);

    totalSizeWritten += bytesCount;

    fflush(file);

    return OK;
}

bool WaveCreator::isEnough(UINT64 bytesInSecond, double secondsNeed) {
    return totalSizeWritten >= bytesInSecond * secondsNeed;
}

void WaveCreator::destroy() {
    closeFile();
}

void WaveCreator::closeFile() {
    if (file)
        fclose(file);
}
