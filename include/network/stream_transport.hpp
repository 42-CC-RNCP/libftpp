#pragma once
#include "endpoint.hpp"
#include <cstddef>
#include <cstdio>

// interface for TCP protocal
// focus on socket/OS operation details
struct IStreamTransport
{
    virtual void connect(const Endpoint& ep) = 0;
    virtual void disconnect() = 0;

    virtual ssize_t sendBytes(const std::byte* data, size_t len) = 0;
    virtual ssize_t recvBytes(std::byte* buf, size_t len) = 0;

    virtual bool isConnected() const = 0;
    virtual ~IStreamTransport() {}
};
