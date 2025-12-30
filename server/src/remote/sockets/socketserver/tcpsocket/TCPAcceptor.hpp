#pragma once
#include <winsock2.h>
#include <memory>

#include "remote/sockets/socketserver/IAcceptor.hpp"
#include "remote/sockets/socketserver/IConnection.hpp"

class TcpAcceptor final : public IAcceptor {
public:
    explicit TcpAcceptor(uint16_t port);
    ~TcpAcceptor() noexcept override;

    std::shared_ptr<IConnection> accept() override;
    void showHostInfo() override;
    void stop() override;

private:
    void startListening();
    static std::string lastErrMsg();

    uint16_t _port;
    SOCKET _listenSocket;
    bool _wsaStarted;
};
