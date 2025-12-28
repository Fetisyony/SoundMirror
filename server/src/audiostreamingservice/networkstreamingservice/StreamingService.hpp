#pragma once

#include <memory>

#include <windows.h>

#include "ErrorCodes.hpp"
#include "audiostreamingservice/consumer/IConsumerService.hpp"
#include "config/ConfigManager.hpp"
#include "network/socketclient/UDPSocketClient.hpp"
#include "network/socketserver/ISocketServer.hpp"

using namespace std;

typedef unsigned char BYTE;

class StreamingService final : public IConsumerService {
public:
    explicit StreamingService(int port);

    errcode_t initialize(WAVEFORMATEX *format) override;

    errcode_t consumeNewData(BYTE *data, UINT32 bytesCount) override;

    void destroy() override;

    ~StreamingService() override;
private:
    std::shared_ptr<ISocketServer> _server{};
    int _port;
    std::shared_ptr<UDPSocketClient> _client{};

    bool _convertEndianess = ConfigManager::getInstance().getConfig().convertEndianess;
    WAVEFORMATEX *_format{};
    int64_t _nextPacketTimestamp = -1;

    errcode_t start();
    void showHostInfo() const;
    errcode_t announceFormat();
    errcode_t sendShort(unsigned short input_little_end);
};
