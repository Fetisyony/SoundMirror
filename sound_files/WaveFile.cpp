#include "WaveFile.h"
#include <assert.h>


WaveCreator::WaveCreator(WAVEFORMATEX *&format) {
    this->format = format;

    int rc = initWaveFile();

    if (rc != OK) {
        // TODO: throw an exception?
        cout << "Error while initializing wave" << endl;
    }
}

int WaveCreator::initWaveFile() {
    file = fopen(filename, "wb");
    if (file == NULL) {
        cout << "Error while opening file" << endl;
        return ERROR;
    }

    WAVEFORMATEXTENSIBLE *pWaveFormatExtensible;
    pWaveFormatExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(format);
    assert(pWaveFormatExtensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

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
int WaveCreator::addSound(BYTE *data, UINT32 new_data_size) {
    if (file == NULL) {
        cout << "Error while accessing file in write wav data" << endl;
        return ERROR_FILE_ACCESS;
    }

    int new_full_data_size = new_data_size + written_data_size;
    int file_size = HEADER_SIZE + new_full_data_size;

    fseek(file, 4, SEEK_SET);
    fwrite(&file_size, 4, 1, file);

    fseek(file, 64, SEEK_SET);
    fwrite(&new_full_data_size, 4, 1, file);

    fseek(file, HEADER_SIZE + written_data_size, SEEK_SET);
    fwrite(data, new_data_size, 1, file);

    written_data_size += new_data_size;

    fflush(file);

    return OK;
}

bool WaveCreator::isEnough(UINT64 bytesInSecond, double secondsNeed) {
    return written_data_size >= bytesInSecond * secondsNeed;
}

int WaveCreator::closeFile() {
    if (file)
        fclose(file);
    return OK;
}
