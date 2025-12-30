#include "StreamingService.hpp"

#include <cstring>

#include <ws2tcpip.h>

#include "remote/sockets/byteorder/ByteOrder.hpp"
#include "remote/sockets/exceptions/NetworkExceptions.hpp"
#include "remote/sockets/socketserver/tcpsocket/TCPAcceptor.hpp"
#include "remote/soundstreamingservice/recorder/utils.hpp"
#include "remote/sockets/socketserver/tcpsocket/TcpConnection.hpp"

StreamingService::StreamingService(int port) : _port(port) {
    _server = std::make_shared<TcpAcceptor>(_port);
    _packetBuffer.resize(2048);
}

errcode_t StreamingService::initialize(WAVEFORMATEX *format) {
    _format = format;

    _managerSocket = _server->accept();

    announceFormat();

    uint32_t streamPort;
    _managerSocket->recvAll(&streamPort, 4);
    spdlog::info("Accepted udp port: {}", streamPort);

    _streamClientSocket = std::make_shared<UDPSocketClient>(_managerSocket->peerAddress(), streamPort);

    return OK;
}

int StreamingService::checkAction() {
    uint32_t value;
    try {
        _managerSocket->recvAll(&value, 4);
        spdlog::info("Accepted new message from manager socket: {}", value);
    } catch (network::ClosedException &e) {
        spdlog::error("recvAll got closed exception on manager socket: {}", value);
        value = 0;
    } catch (network::SocketException &e) {
        spdlog::error("recvAll got closed exception on manager socket: {}", value);
        value = 0;
    }
    return value;
}

void StreamingService::announceFormat() {
    sendShort(_format->nChannels);
    sendShort(_format->wBitsPerSample / 8);
    sendShort(static_cast<unsigned short>(_format->nSamplesPerSec));
}

errcode_t StreamingService::consumeNewData(BYTE *data, UINT32 bytesCount) {
    errcode_t rc = OK;

    UINT32 blockAlign = (_format->nBlockAlign > 0) ? _format->nBlockAlign : 4;
    size_t effectiveMaxPayload = MAX_AUDIO_PER_PACKET - (MAX_AUDIO_PER_PACKET % blockAlign);

    if (_convertEndianess)
        byteorder::swapSoundEndianess(data, bytesCount / blockAlign, _format);

    UINT32 offset = 0;
    while (offset < bytesCount) {
        UINT32 remaining = bytesCount - offset;
        UINT32 currentChunkSize = (remaining > effectiveMaxPayload) ? effectiveMaxPayload : remaining;
        UINT32 totalPacketSize = CUSTOM_HEADER_SIZE + currentChunkSize;

        std::memcpy(_packetBuffer.data(), &currentChunkSize, sizeof(currentChunkSize));
        std::memcpy(_packetBuffer.data() + 4, &_id, 8);
        std::memcpy(_packetBuffer.data() + 12, data + offset, currentChunkSize);

        _streamClientSocket->sendMessage(_packetBuffer.data(), totalPacketSize);

        offset += currentChunkSize;
        ++_id;
    }

    return rc;
}

void StreamingService::sendShort(unsigned short input_little_end) {
    unsigned short input_big_end = byteorder::swapEndianess(input_little_end);
    BYTE *buf = reinterpret_cast<BYTE *>(&input_big_end);
    _managerSocket->send(buf, sizeof(input_big_end));
}

void StreamingService::showHostInfo() const {
    char hostname[1024];
    gethostname(hostname, 1024);
    hostent *host = gethostbyname(hostname);
    char *ip = inet_ntoa(*(struct in_addr *) host->h_addr_list[0]);
    spdlog::info("Server listening on port {}", _port);
    spdlog::info("Host IP: {}", ip);
}

void StreamingService::destroy() {
    _server->stop();
    _streamClientSocket->closeSocket();
}

StreamingService::~StreamingService() {}
