#pragma once
#include <functional>
#include <memory>

struct IConnection;
using ConnectionPtr = std::shared_ptr<IConnection>;
using AcceptHandler = std::function<void(ConnectionPtr)>;

struct IAcceptor {
    virtual ~IAcceptor() = default;
    virtual std::shared_ptr<IConnection> accept() = 0;
    virtual void showHostInfo() = 0;
    virtual void stop() = 0;
};
