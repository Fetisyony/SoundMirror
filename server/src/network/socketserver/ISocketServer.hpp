#pragma once
#include "ErrorCodes.hpp"

class ISocketServer {
public:
    virtual ~ISocketServer() = default;

    virtual errcode_t init(int port) = 0;

    virtual errcode_t start() = 0;

    virtual errcode_t stop() = 0;

    virtual void showHostInfo() = 0;

    virtual errcode_t sendMessage(BYTE *message, UINT32 size) = 0;
};
