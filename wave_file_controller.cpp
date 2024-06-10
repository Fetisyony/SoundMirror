#include "wave_file_controller.h"


DWORD cksize = sizeof(WAVEFORMATEXTENSIBLE);
int HEADER_SIZE = 4 + 4 + 4 + 4 + 4 + cksize + 4 + 4;  // 68


const char *filename = "output/test.wav";

FILE *file = NULL;
UINT64 written_data_size = 0;

int initWavFile(WAVEFORMATEX *format) {
    file = fopen(filename, "wb");
    if (file == NULL) {
        std::cout << "Error while opening file" << std::endl;
        return FILE_ERROR;
    }

    WAVEFORMATEXTENSIBLE *pWaveFormatExtensible;
    pWaveFormatExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(format);

    assert (pWaveFormatExtensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

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

    return OK;
}

int writeWavData(BYTE *data, UINT32 new_data_size) {
    if (file == NULL) {
        std::cout << "Error while accessing file in write wav data" << std::endl;
        return FILE_ERROR;
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

    return OK;
}

bool is_enough(UINT64 bytes_in_second) {
    return written_data_size >= bytes_in_second * RECORDING_DURATION_SECONDS;
}

int close_file() {
    if (file == NULL) {
        std::cout << "Error while closing unexisting file" << std::endl;
        return FILE_ERROR;
    }
    fclose(file);
    return OK;
}
