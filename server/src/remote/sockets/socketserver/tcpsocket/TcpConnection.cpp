#include <winsock2.h>
#include "TcpConnection.hpp"

#include "remote/sockets/exceptions/NetworkExceptions.hpp"
#include "remote/sockets/networkanalyzer/NetworkAnalyzer.hpp"
#include "spdlog/spdlog.h"

#ifdef _WIN32
# define CLOSE_SOCKET(s) ::closesocket(s)
#else
# define CLOSE_SOCKET(s) ::close(s)
#endif

TcpConnection::TcpConnection(SOCKET s, sockaddr_in peer)
    : _socket(s), _peer(peer), _closed(false) {}

TcpConnection::~TcpConnection() noexcept {
    try { close(); } catch (...) {}
}

ssize_t TcpConnection::send(const void* data, size_t len) {
    if (_closed) throw network::ClosedException("send", peerAddress());
    if (!data || len == 0) return 0;
    int r = ::send(_socket, reinterpret_cast<const char*>(data), static_cast<int>(len), 0);
    if (r == SOCKET_ERROR) {
        spdlog::error("[tcp_conn] send to {} failed: {}", peerAddress(), lastErrMsg());
        throw network::SendException("send", peerAddress());
    }
    return r;
}

ssize_t TcpConnection::recv(void* buf, size_t len) {
    if (_closed) throw network::ClosedException("recv", peerAddress());
    if (!buf || len == 0) return 0;
    int r = ::recv(_socket, reinterpret_cast<char*>(buf), static_cast<int>(len), 0);
    if (r == SOCKET_ERROR) {
        spdlog::error("[tcp_conn] recv from {} failed: {}", peerAddress(), lastErrMsg());
        throw network::ReceiveException("recv", peerAddress());
    }
    if (r == 0) {
        spdlog::info("[tcp_conn] peer {} disconnected (recv returned 0)", peerAddress());
        throw network::ClosedException("recv", peerAddress());
    }
    return static_cast<ssize_t>(r);
}

ssize_t TcpConnection::recvAll(void *data, int size) {
    if (_closed) throw network::ClosedException("recvAll", peerAddress());
    if (!data || size <= 0) return 0;
    char *buffer = static_cast<char*>(data);
    int total = 0;
    while (total < size) {
        int got = ::recv(_socket, buffer + total, size - total, 0);
        if (got == SOCKET_ERROR) {
            spdlog::error("[tcp_conn] recvAll from {} failed: {}", peerAddress(), lastErrMsg());
            throw network::ReceiveException("recvAll", peerAddress());
        }
        if (got == 0) {
            spdlog::info("[tcp_conn] peer {} disconnected during recvAll", peerAddress());
            throw network::ClosedException("recvAll", peerAddress());
        }
        total += got;
    }
    return total;
}

ssize_t TcpConnection::recvMessage(byte *buffer, int32_t bufferSize, int32_t &bytes_received) {
    bytes_received = 0;
    if (_closed) throw network::ClosedException("recvMessage(buf)", peerAddress());
    if (!buffer || bufferSize <= 0) throw network::ReceiveException("recvMessage(buf)", peerAddress());
    int r = ::recv(_socket, reinterpret_cast<char*>(buffer), bufferSize, 0);
    if (r == SOCKET_ERROR) {
        spdlog::error("[tcp_conn] recvMessage(buf) from {} failed: {}", peerAddress(), lastErrMsg());
        throw network::ReceiveException("recvMessage(buf)", peerAddress());
    }
    if (r == 0) {
        spdlog::info("[tcp_conn] peer {} disconnected (recvMessage buf)", peerAddress());
        throw network::ClosedException("recvMessage(buf)", peerAddress());
    }
    bytes_received = r;
    return static_cast<ssize_t>(r);
}

ssize_t TcpConnection::recvMessage(std::vector<byte> &message) {
    if (_closed) throw network::ClosedException("recvMessage(vec)", peerAddress());
    uint32_t network_size = 0;
    recvAll(&network_size, static_cast<int>(sizeof(network_size)));
    uint32_t size = ntohl(network_size);
    constexpr uint32_t MAX_MSG_SIZE = 10u * 1024u * 1024u;
    if (size > MAX_MSG_SIZE) {
        spdlog::error("[tcp_conn] incoming message size {} exceeds limit", size);
        throw network::ProtocolException("recvMessage(vec)", peerAddress());
    }
    message.resize(size);
    recvAll(message.data(), static_cast<int>(size));
    return size;
}

void TcpConnection::close() {
    if (_closed.exchange(true)) return;
    CLOSE_SOCKET(_socket);
    spdlog::info("[tcp_conn] closed connection to {}", peerAddress());
}

std::string TcpConnection::peerAddress() const {
    return NetworkAnalyzer::sockAddrToString(_peer).first;
}

std::string TcpConnection::lastErrMsg() {
#ifdef _WIN32
    return std::to_string(WSAGetLastError());
#else
    return std::string(strerror(errno));
#endif
}