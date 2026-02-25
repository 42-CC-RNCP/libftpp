// network/contracts/reactor.hpp
#pragma once
#include <functional>

enum class IoEvent
{
    Readable = 1 << 0,
    Writable = 1 << 1,
    Error = 1 << 2,
    Closed = 1 << 3,
};

inline bool hasEvent(IoEvent events, IoEvent check)
{
    return (static_cast<int>(events) & static_cast<int>(check)) != 0;
}

using IoCallback = std::function<void(int fd, IoEvent events)>;

class IReactor
{
public:
    virtual ~IReactor() = default;

    virtual void add(int fd, IoEvent events, IoCallback cb) = 0;
    virtual void modify(int fd, IoEvent events) = 0;
    virtual void remove(int fd) = 0;

    virtual int poll(int timeout_ms = -1) = 0;
};
