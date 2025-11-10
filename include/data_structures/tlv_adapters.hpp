// include/data_structures/tlv_adapters.hpp
#pragma once
#include "data_buffer.hpp"
#include "design_patterns/memento/snapio.hpp"
#include "tlv.hpp"

// ---------------------------
// DataBuffer <</>>
// ---------------------------

class DataBuffer;

// DataBuffer or any others *Buffer do not need to know TLV
// register in the header

// format:
//  template <class T> *Buffer& operator<<(*Buffer& out, const T& v)
//  template <class T> *Buffer& operator>>(*Buffer& in, T& v)

template <class T> DataBuffer& operator<<(DataBuffer& out, const T& v)
{
    tlv::write_value(out, v);
    return out;
}

template <class T> DataBuffer& operator>>(DataBuffer& in, T& v)
{
    tlv::read_value(in, v);
    return in;
}

namespace tlv_adapt
{

// ---------------------------
// SnapIO <</>>
// ---------------------------
// warpper to adapt SnapIO to ByteWriter/ByteReader concept
struct SnapIOWriter
{
    SnapIO& io;
    void writeBytes(std::span<const std::byte> s)
    {
        io.write(s.data(), s.size());
    }
};

struct SnapIOReader
{
    SnapIO& io;
    void readExact(std::byte* p, std::size_t n) { io.read(p, n); }
};

template <class T> inline SnapIO& operator<<(SnapIO& io, const T& v)
{
    SnapIOWriter writer{io};
    tlv::write_value(writer, v);
    return io;
}
template <class T> inline SnapIO& operator>>(SnapIO& io, T& v)
{
    SnapIOReader reader{io};
    tlv::read_value(reader, v);
    return io;
}

} // namespace tlv_adapt
