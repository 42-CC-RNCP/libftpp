#pragma once
#include "stream_transport.hpp"
#include <cstdint>
#include <memory>

class IAcceptor
{
public:
    virtual ~IAcceptor() = default;
    virtual void listen(std::uint16_t port) = 0;

    // try to accept a new connection, return nullptr if no pending connection
    virtual std::unique_ptr<IStreamTransport> tryAccept() = 0;

    // get the native handle of the acceptor to add to the event loop
    virtual int nativeHandle() const = 0;
};
