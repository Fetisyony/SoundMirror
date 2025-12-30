#pragma once

#include "remote/sockets/socketclient/UDPSocketClient.hpp"

#include <memory>
#include <windows.h>

#include "ErrorCodes.hpp"
#include "remote/soundstreamingservice/consumerservice/IConsumerService.hpp"
#include "config/ConfigManager.hpp"
#include "remote/sockets/socketserver/IAcceptor.hpp"

typedef unsigned char BYTE;

class StreamingService final : public IConsumerService {
public:
    explicit StreamingService(int port);

    errcode_t initialize(WAVEFORMATEX *format) override;

    int checkAction() override;

    errcode_t consumeNewData(BYTE *data, UINT32 bytesCount) override;

    void destroy() override;

    ~StreamingService() override;
private:
    std::shared_ptr<IAcceptor> _server{};
    std::shared_ptr<IConnection> _managerSocket{};
    int _port;
    std::shared_ptr<UDPSocketClient> _streamClientSocket{};
    std::vector<BYTE> _packetBuffer;

    bool _convertEndianess = ConfigManager::getInstance().getConfig().convertEndianess;
    WAVEFORMATEX *_format{};
    int64_t _id = 0;

    const size_t MAX_UDP_PAYLOAD = 1024;
    const size_t CUSTOM_HEADER_SIZE = 12;
    const size_t MAX_AUDIO_PER_PACKET = MAX_UDP_PAYLOAD - CUSTOM_HEADER_SIZE;

    void showHostInfo() const;
    void announceFormat();
    void sendShort(unsigned short input_little_end);
};
