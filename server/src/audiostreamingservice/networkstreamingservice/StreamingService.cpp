#include "StreamingService.hpp"

#include <chrono>
#include <cstring>

#include "mathutils/converters.hpp"
#include "network/socketserver/tcpsocketserver/TCPSocketServer.hpp"

StreamingService::StreamingService(int port, bool convertEndianess) : _port(port), _convertEndianess(convertEndianess) {
    _server = std::make_shared<TCPSocketServer>();
    auto config = SocketConfig(_port);
    _server->init(config);
}

StreamingService::~StreamingService() {
    stop();
}

errcode_t StreamingService::initialize(WAVEFORMATEX *format) {
    _format = format;

    start();

    return OK;
}

errcode_t StreamingService::consumeNewData(BYTE *data, UINT32 bytesCount) {
    errcode_t rc = OK;

    // Convert the audio data from little-endian to big-endian
    if (_convertEndianess)
        swapSoundEndianess(data, bytesCount / _format->nBlockAlign, _format);

    BYTE header[4];
    std::memcpy(header, &bytesCount, sizeof(header));
    if (rc == OK)
        rc = _server->sendMessage(header, sizeof(header));

    const auto now = std::chrono::system_clock::now();
    const int64_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    BYTE timestamp[8];
    std::memcpy(timestamp, &millis, sizeof(timestamp));
    if (rc == OK)
        rc = _server->sendMessage(timestamp, sizeof(timestamp));
    if (rc == OK)
        rc = _server->sendMessage(data, bytesCount);

    return rc;
}

void StreamingService::destroy() {
    stop();
}

errcode_t StreamingService::start() {
    auto rc = _server->start();
    if (rc == OK)
        announceFormat();
    else
        std::cout << "Error starting" << std::endl;
    return rc;
}

errcode_t StreamingService::stop() {
    return _server->stop();
}

errcode_t StreamingService::announceFormat() {
    errcode_t rc = OK;

    if (rc == OK)
        rc = sendShort(_format->nChannels);

    if (rc == OK)
        rc = sendShort(_format->wBitsPerSample / 8);

    if (rc == OK)
        rc = sendShort(static_cast<unsigned short>(_format->nSamplesPerSec));

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
