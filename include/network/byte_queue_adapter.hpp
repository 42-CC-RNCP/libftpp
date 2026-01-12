#pragma once
#include "byte_queue.hpp"
#include "data_structures/data_buffer.hpp"

class DataBufferByteQueue : public ByteQueue
{
public:
    const std::byte* data() const override { return b_.data(); }

    std::size_t remaining() const override { return b_.remaining(); }

    std::span<const std::byte> peek() const override
    {
        return std::span<const std::byte>(b_.data(), b_.remaining());
    }

    void append(std::span<const std::byte> s) override { b_.writeBytes(s); }
    void consume(std::size_t n) override
    {
        b_.consume(n);

        // compact the buffer if more than half is consumed
        if (b_.tell() > b_.size() / 2) {
            b_.compact();
        }
    }
    void compact() override { b_.compact(); }

private:
    DataBuffer b_;
};
