#pragma once

class IConsumerService {
public:
    virtual ~IConsumerService() = default;

    virtual errcode_t initialize(WAVEFORMATEX *format) = 0;

    virtual errcode_t consumeNewData(BYTE *data, UINT32 bytesCount) = 0;

    virtual void destroy() = 0;
};
