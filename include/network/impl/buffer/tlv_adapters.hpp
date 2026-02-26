// network/impl/buffer/tlv_adapters.hpp
#pragma once
#include <span>
#include <stdexcept>
#include <cstring>
#include "data_structures/tlv.hpp"
#include "byte_queue_adapter.hpp"

namespace tlv_adapt
{

// ---------------------------
// Byte Queue <</>>
// ---------------------------
// warpper to adapt ByteQueue to ByteWriter/ByteReader concept

struct ByteQueueWriter
{
    DataBufferByteQueue& q;
    void writeBytes(std::span<const std::byte> s) { q.append(s); }
};

struct ByteQueueReader
{
    DataBufferByteQueue& q;

    void readExact(std::byte* p, std::size_t n)
    {
        std::size_t off = 0;
        while (off < n) {
            auto sp = q.peek();
            if (sp.empty()) {
                throw std::runtime_error("need more data");
            }
            const std::size_t take = std::min(sp.size(), n - off);
            std::memcpy(p + off, sp.data(), take);
            q.consume(take);
            off += take;
        }
    }
};

template <class T>
inline DataBufferByteQueue& operator<<(DataBufferByteQueue& out, const T& v)
{
    tlv_adapt::ByteQueueWriter w{out};
    tlv::write_value(w, v);
    return out;
}

template <class T>
inline DataBufferByteQueue& operator>>(DataBufferByteQueue& in, T& v)
{
    tlv_adapt::ByteQueueReader r{in};
    tlv::read_value(r, v);
    return in;
}

} // namespace tlv_adapt
