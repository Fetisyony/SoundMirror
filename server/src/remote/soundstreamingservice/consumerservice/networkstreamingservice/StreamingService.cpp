#include "StreamingService.hpp"

#include <cstring>
#include <iostream>

#include <opus.h>
#include <ws2tcpip.h>

#include "remote/sockets/byteorder/ByteOrder.hpp"
#include "remote/sockets/exceptions/NetworkExceptions.hpp"
#include "remote/sockets/socketserver/tcpsocket/TCPAcceptor.hpp"
#include "remote/soundstreamingservice/recorder/utils.hpp"
#include "remote/sockets/socketserver/tcpsocket/TcpConnection.hpp"

StreamingService::StreamingService(int port) : _port(port) {
    _server = std::make_shared<TcpAcceptor>(_port);
    _packetBuffer.resize(512);
}

errcode_t StreamingService::initialize(WAVEFORMATEX *format) {
    _pcmBuffer.reserve(16384);
    _format = format;

    int err = 0;
    enc = opus_encoder_create(_format->nSamplesPerSec, _format->nChannels, OPUS_APPLICATION_AUDIO, &err);
    if (!enc || err != OPUS_OK) {
        throw std::runtime_error(std::string("opus_encoder_create failed: ") + opus_strerror(err));
    }
    opus_int32 bitrate = 96000;

    if (_format->nChannels == 2) {
        bitrate = (_format->nSamplesPerSec <= 16000) ? 40000 : 96000;
    } else {
        bitrate = (_format->nSamplesPerSec <= 16000) ? 24000 : 64000;
    }

    opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate));
    opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(10));
    opus_encoder_ctl(enc, OPUS_SET_VBR(1));

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

    _pcmBuffer.insert(_pcmBuffer.end(), data, data + bytesCount);

    int frameSizeSamples = _format->nSamplesPerSec / 100;
    uint32_t bytesPerFrame = frameSizeSamples * _format->nBlockAlign;
    // uint32_t effectiveMaxPayload = MAX_AUDIO_PER_PACKET;

    byte outbuf[1500];
    while (_pcmBuffer.size() - _readPos >= bytesPerFrame) {

        int compressedBytesN = opus_encode_float(
                    enc,
                    (const float *)(_pcmBuffer.data() + _readPos),
                    frameSizeSamples,
                    outbuf,
                    sizeof(outbuf)
                );

        if (compressedBytesN < 0) {
            spdlog::error("opus_encode_float failed: {}", opus_strerror(compressedBytesN));
            _readPos += bytesPerFrame;
            continue;
        }

        uint32_t currentChunkSize = compressedBytesN;
        uint32_t totalPacketSize = CUSTOM_HEADER_SIZE + currentChunkSize;
        uint32_t swappedCurrentChunkSize = byteorder::swapEndianess32(currentChunkSize);
        uint64_t swappedId = byteorder::swapEndianess64(_id);

        std::memcpy(_packetBuffer.data(), &swappedCurrentChunkSize, sizeof(currentChunkSize));
        std::memcpy(_packetBuffer.data() + 4, &swappedId, sizeof(swappedId));
        std::memcpy(_packetBuffer.data() + 12, outbuf, currentChunkSize);

        _streamClientSocket->sendMessage(_packetBuffer.data(), totalPacketSize);
        ++_id;

        _readPos += bytesPerFrame;
    }

    if (_readPos == _pcmBuffer.size()) {
        _pcmBuffer.clear();
        _readPos = 0;
    } else if (_readPos > 10000) {
        _pcmBuffer.erase(_pcmBuffer.begin(), _pcmBuffer.begin() + _readPos);
        _readPos = 0;
    }

    return rc;
}

void StreamingService::sendShort(unsigned short input_little_end) {
    unsigned short input_big_end = byteorder::swapEndianess16(input_little_end);
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
