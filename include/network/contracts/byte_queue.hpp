// network/contracts/byte_queue.hpp
#pragma once
#include <cstddef>
#include <span>

class ByteQueue
{
public:
    virtual ~ByteQueue() = default;
    virtual const std::byte* data() const = 0;
    virtual std::size_t remaining() const = 0;
    virtual std::span<const std::byte> peek() const = 0; // view front
    virtual void append(std::span<const std::byte> s) = 0;
    virtual void consume(std::size_t n) = 0;
    virtual void compact() = 0;
};
