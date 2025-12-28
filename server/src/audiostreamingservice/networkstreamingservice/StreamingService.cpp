#include "StreamingService.hpp"

#include <chrono>
#include <cstring>

#include "audiostreamingservice/recorder/utils.hpp"
#include "mathutils/converters.hpp"
#include "network/socketserver/tcpsocketserver/TCPSocketServer.hpp"

StreamingService::StreamingService(int port) : _port(port) {
    _server = std::make_shared<TCPSocketServer>();
    auto config = SocketConfig(_port);
    _server->init(config);
}

errcode_t StreamingService::start() {
    auto rc = _server->start();

    UINT32 streamPort;
    _server->recvAll(&streamPort, 4);
    std::cout << streamPort << std::endl;

    char buf[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &_server->getClient().sin_addr, buf, sizeof(buf));
    _client = std::make_shared<UDPSocketClient>();
    rc = _client->init(std::string(buf), streamPort);

    if (rc == OK)
        announceFormat();
    else
        std::cout << "Error starting" << std::endl;
    return rc;
}

errcode_t StreamingService::announceFormat() {
    errcode_t rc = OK;

    printFormat(_format);

    if (rc == OK)
        rc = sendShort(_format->nChannels);

    if (rc == OK)
        rc = sendShort(_format->wBitsPerSample / 8);

    if (rc == OK)
        rc = sendShort(static_cast<unsigned short>(_format->nSamplesPerSec));

    return rc;
}

errcode_t StreamingService::initialize(WAVEFORMATEX *format) {
    _format = format;

    start();

    return OK;
}

const size_t MAX_UDP_PAYLOAD = 1400;
const size_t CUSTOM_HEADER_SIZE = 12;
const size_t MAX_AUDIO_PER_PACKET = MAX_UDP_PAYLOAD - CUSTOM_HEADER_SIZE;

errcode_t StreamingService::consumeNewData(BYTE *data, UINT32 bytesCount) {
    errcode_t rc = OK;

    UINT32 blockAlign = (_format->nBlockAlign > 0) ? _format->nBlockAlign : 4;
    size_t effectiveMaxPayload = MAX_AUDIO_PER_PACKET - (MAX_AUDIO_PER_PACKET % blockAlign);

    if (_convertEndianess)
        swapSoundEndianess(data, bytesCount / blockAlign, _format);

    if (_nextPacketTimestamp == -1) {
        const auto now = std::chrono::system_clock::now();
        _nextPacketTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }

    double bytesPerMillis = (double)_format->nAvgBytesPerSec / 1000.0;
    if (bytesPerMillis <= 0.0) bytesPerMillis = 44.1 * blockAlign;

    size_t offset = 0;
    std::vector<BYTE> packetBuffer(2048);
    while (offset < bytesCount) {
        size_t remaining = bytesCount - offset;
        size_t currentChunkSize = (remaining > effectiveMaxPayload) ? effectiveMaxPayload : remaining;
        size_t totalPacketSize = CUSTOM_HEADER_SIZE + currentChunkSize;

        UINT32 sizeToSend = (UINT32)currentChunkSize;

        std::memcpy(packetBuffer.data(), &sizeToSend, sizeof(sizeToSend));

        std::memcpy(packetBuffer.data() + 4, &_nextPacketTimestamp, 8);

        std::memcpy(packetBuffer.data() + 12, data + offset, currentChunkSize);

        rc = _client->sendMessage(packetBuffer.data(), totalPacketSize);
        if (rc != OK) {
            std::cout << "Error sending UDP packet" << std::endl;
        }

        offset += currentChunkSize;

        double durationMs = (double)currentChunkSize / bytesPerMillis;
        _nextPacketTimestamp += (int64_t)(durationMs + 0.5);
    }
    return rc;
}

errcode_t StreamingService::sendShort(unsigned short input_little_end) {
    unsigned short input_big_end = swapEndianess(input_little_end);
    BYTE *buf = reinterpret_cast<BYTE *>(&input_big_end);

    int bytesSent = _server->sendMessage(buf, sizeof(input_big_end));
    if (bytesSent == SOCKET_ERROR) {
        cout << "Error sending data." << input_little_end << endl;
        return SOCKET_ERROR;
    }
    return OK;
}

void StreamingService::showHostInfo() const {
    char hostname[1024];
    gethostname(hostname, 1024);
    hostent *host = gethostbyname(hostname);
    char *ip = inet_ntoa(*(struct in_addr *) host->h_addr_list[0]);
    printf("Server listening on port %d\n", _port);
    printf("Host IP: %s\n", ip);
}

void StreamingService::destroy() {
    _server->stop();
    _client->closeSocket();
}

StreamingService::~StreamingService() {
    destroy();
}
