// include/data_structures/memento/tlv_adapters.hpp
#include "data_structures/tlv.hpp"
#include "snapio.hpp"
#include <span>

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
