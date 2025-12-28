#pragma once
#include <vector>

#include "ErrorCodes.hpp"
#include "SocketConfig.hpp"

class ISocketServer {
public:
    virtual ~ISocketServer() = default;

    virtual errcode_t init(const SocketConfig &config) = 0;

    virtual errcode_t start() = 0;

    virtual errcode_t stop() = 0;

    virtual void showHostInfo() = 0;

    virtual errcode_t sendMessage(const BYTE *message, UINT32 size) = 0;

    virtual errcode_t recvAll(void *data, int size) = 0;
    virtual errcode_t recvMessage(BYTE *buffer, UINT32 bufferSize, UINT32 &bytes_received) = 0;
    virtual errcode_t recvMessage(vector<BYTE> &message) = 0;

    virtual sockaddr_in& getClient() = 0;
};
