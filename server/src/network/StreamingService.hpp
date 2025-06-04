#pragma once

#include <memory>

#include <windows.h>

#include "ErrorCodes.hpp"
#include "consumer/IConsumerService.hpp"
#include "socketserver/ISocketServer.hpp"

using namespace std;

typedef unsigned char BYTE;

class StreamingService final : public IConsumerService {
public:
    explicit StreamingService(int port, bool convertEndianess = false);
    ~StreamingService() override;

    errcode_t initialize(WAVEFORMATEX *format) override;

    errcode_t consumeNewData(BYTE *data, UINT32 bytesCount) override;

    void destroy() override;

private:
    std::shared_ptr<ISocketServer> _server{};
    int _port;
    bool _convertEndianess;
    WAVEFORMATEX *_format{};

    errcode_t start();
    errcode_t stop();
    void showHostInfo() const;
    errcode_t announceFormat();
    errcode_t sendShort(unsigned short input_little_end);
};
