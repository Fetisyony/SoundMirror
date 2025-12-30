#include "TCPAcceptor.hpp"

#include <cstdint>
#include <memory>
#include <psdk_inc/_socket_types.h>

#include "TcpConnection.hpp"
#include "remote/sockets/exceptions/NetworkExceptions.hpp"
#include "remote/sockets/networkanalyzer/NetworkAnalyzer.hpp"
#include "remote/sockets/socketserver/IConnection.hpp"
#include "spdlog/spdlog.h"

#ifdef _WIN32
# define CLOSE_SOCKET(s) ::closesocket(s)
#else
# define CLOSE_SOCKET(s) ::close(s)
#endif

TcpAcceptor::TcpAcceptor(uint16_t port)
    : _port(port), _listenSocket(INVALID_SOCKET), _wsaStarted(false) {
    startListening();
}

TcpAcceptor::~TcpAcceptor() noexcept {
    try { stop(); } catch (...) {}
}

std::shared_ptr<IConnection> TcpAcceptor::accept() {
    if (_listenSocket == INVALID_SOCKET) {
        spdlog::error("[tcp_acceptor] accept() called but listen socket invalid");
        throw network::AcceptException("accept", "0.0.0.0:" + std::to_string(_port));
    }

    sockaddr_in clientAddr{};
#ifdef _WIN32
    int clientAddrSize = static_cast<int>(sizeof(clientAddr));
#else
    socklen_t clientAddrSize = static_cast<socklen_t>(sizeof(clientAddr));
#endif

    SOCKET clientSock = ::accept(_listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
    if (clientSock == INVALID_SOCKET) {
        spdlog::error("[tcp_acceptor] accept failed: {}", lastErrMsg());
        throw network::AcceptException("accept", "0.0.0.0:" + std::to_string(_port));
    }

    spdlog::info("[tcp_acceptor] accepted client {}", NetworkAnalyzer::sockAddrToString(clientAddr).first + std::to_string(NetworkAnalyzer::sockAddrToString(clientAddr).second));
    return std::make_shared<TcpConnection>(clientSock, clientAddr);
}

void TcpAcceptor::showHostInfo() {
    spdlog::info("[tcp_acceptor] listening on port {}", _port);
    std::string ip;
    try {
        ip = NetworkAnalyzer::getLocalLanIpAddress();
    } catch (...) {
        ip.clear();
    }
    if (!ip.empty()) {
        spdlog::info("[tcp_acceptor] host LAN IP: {}", ip);
    } else {
        spdlog::warn("[tcp_acceptor] could not determine LAN IP");
    }
}

void TcpAcceptor::stop() {
    if (_listenSocket != INVALID_SOCKET) {
        CLOSE_SOCKET(_listenSocket);
        _listenSocket = INVALID_SOCKET;
        spdlog::info("[tcp_acceptor] listen socket closed");
    }
    if (_wsaStarted) {
#ifdef _WIN32
        ::WSACleanup();
#endif
        _wsaStarted = false;
        spdlog::info("[tcp_acceptor] WSA cleaned up");
    }
}

void TcpAcceptor::startListening() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        spdlog::error("[tcp_acceptor] WSAStartup failed");
        throw network::ResourceException("WSAStartup", "0.0.0.0:" + std::to_string(_port));
    }
    _wsaStarted = true;
#endif

    _listenSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_listenSocket == INVALID_SOCKET) {
        spdlog::error("[tcp_acceptor] socket() failed");
        throw network::ResourceException("socket", "0.0.0.0:" + std::to_string(_port));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);

    if (::bind(_listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        spdlog::error("[tcp_acceptor] bind() failed: {}", lastErrMsg());
        throw network::BindException("bind", "0.0.0.0:" + std::to_string(_port));
    }

    if (::listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        spdlog::error("[tcp_acceptor] listen() failed: {}", lastErrMsg());
        throw network::ListenException("listen", "0.0.0.0:" + std::to_string(_port));
    }

    showHostInfo();
}

std::string TcpAcceptor::lastErrMsg() {
#ifdef _WIN32
    return std::to_string(WSAGetLastError());
#else
    return std::string(strerror(errno));
#endif
}
