#pragma once
#include <cstdint>
#include <string>
#include <vector>

typedef unsigned char byte;

struct IConnection {
    virtual ~IConnection() = default;
    virtual ssize_t send(const void* data, size_t len) = 0;
    virtual ssize_t recv(void* buf, size_t len) = 0;
    virtual ssize_t recvAll(void *data, int size) = 0;
    virtual ssize_t recvMessage(byte *buffer, int32_t bufferSize, int32_t &bytes_received) = 0;
    virtual ssize_t recvMessage(std::vector<byte> &message) = 0;
    virtual void close() = 0;
    virtual std::string peerAddress() const = 0;
};
