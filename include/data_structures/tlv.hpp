#pragma once
#include "tlv_io.hpp"
#include "tlv_type_traits.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace tlv
{

enum class WireType : std::uint8_t
{
    VarUInt = 0,
    VarSIntZigZag = 1,
    Bytes = 2,
    Fixed32 = 3,
    Fixed64 = 4,
};

constexpr std::uint8_t wire_code(WireType t)
{
    switch (t) {
    case WireType::VarUInt:
        return 0;
    case WireType::VarSIntZigZag:
        return 1;
    case WireType::Bytes:
        return 2;
    case WireType::Fixed32:
        return 3;
    case WireType::Fixed64:
        return 4;
    }
    return 5;
}

template <class Out> inline void write_header(Out& out, WireType t)
{
    const std::byte b{static_cast<std::byte>(wire_code(t) & 0x07)};
    out.writeBytes({&b, 1});
}

template <class In> inline WireType read_header(In& in)
{
    std::byte b;

    in.readExact(&b, 1);
    switch (std::to_integer<uint8_t>(b) & 0x07) {
    case 0:
        return WireType::VarUInt;
    case 1:
        return WireType::VarSIntZigZag;
    case 2:
        return WireType::Bytes;
    case 3:
        return WireType::Fixed32;
    case 4:
        return WireType::Fixed64;
    default:
        throw std::runtime_error("unknown wire");
    }
}

// internal functions
namespace detail
{

// ZigZag codec
inline uint32_t zigzag_encode32(int32_t n)
{
    return (uint32_t(n) << 1) ^ uint32_t(n >> 31);
}

inline uint64_t zigzag_encode64(int64_t n)
{
    return (uint64_t(n) << 1) ^ uint64_t(n >> 63);
}

inline int64_t zigzag_decode64(uint64_t u)
{
    return static_cast<int64_t>((u >> 1) ^ (~(u & 1) + 1));
}

inline int32_t zigzag_decode32(uint32_t u)
{
    return static_cast<int32_t>((u >> 1) ^ (~(u & 1) + 1));
}

// Base-128 varint, support 64-bit type < 10bytes (ceil(64 / 7))
constexpr int kMaxVarint64 = (64 + 6) / 7;

// without zigzag varint only works for unsigned int
template <ByteWriter Out>
inline void write_varint(Out& out, std::uint64_t value)
{
    std::byte tmp[kMaxVarint64];
    size_t i = 0;

    while (value >= 0x80u) {
        tmp[i++] = std::byte((value & 0x7Fu) | 0x80u);
        value >>= 7;
    }
    tmp[i++] = std::byte(value & 0x7Fu);
    out.writeBytes({tmp, i});
}

template <ByteReader In> inline std::uint64_t read_varint(In& in)
{
    std::uint64_t value = 0;
    int shift = 0;

    for (int i = 0; i < kMaxVarint64; i++) {
        std::byte b{};
        in.readExact(&b, 1);

        const auto ub = std::to_integer<std::uint8_t>(b);
        value |= std::uint64_t(ub & 0x7Fu) << shift;
        if (!(ub & 0x80u)) {
            return value;
        }
        shift += 7;
    }
    throw std::runtime_error("varint too long");
}

template <ByteWriter Out>
inline void write_fixed32_le(Out& out, std::uint32_t x)
{
    // convert to little-end byte by byte
    std::byte b[4] = {
        std::byte(x & 0xFFu),
        std::byte((x >> 8) & 0xFFu),
        std::byte((x >> 16) & 0xFFu),
        std::byte((x >> 24) & 0xFFu),
    };
    out.writeBytes({b, 4});
}

template <ByteWriter Out>
 void write_fixed64_le(Out& out, std::uint64_t x)
{
    // convert to little-end byte by byte
    std::byte b[8] = {
        std::byte(x & 0xFFu),
        std::byte((x >> 8) & 0xFFu),
        std::byte((x >> 16) & 0xFFu),
        std::byte((x >> 24) & 0xFFu),
        std::byte((x >> 32) & 0xFFu),
        std::byte((x >> 40) & 0xFFu),
        std::byte((x >> 48) & 0xFFu),
        std::byte((x >> 56) & 0xFFu),
    };
    out.writeBytes({b, 8});
}
template <ByteReader In> inline std::uint32_t read_fixed32_le(In& in)
{
    std::byte b[4];
    in.readExact(b, 4);

    return (std::uint32_t(std::to_integer<unsigned>(b[0])))
           | ((std::uint32_t(std::to_integer<unsigned>(b[1]))) << 8)
           | ((std::uint32_t(std::to_integer<unsigned>(b[2]))) << 16)
           | ((std::uint32_t(std::to_integer<unsigned>(b[3]))) << 24);
}
template <ByteReader In> inline std::uint64_t read_fixed64_le(In& in)
{
    std::byte b[8];
    in.readExact(b, 8);

    return (std::uint64_t(std::to_integer<unsigned>(b[0])))
           | ((std::uint64_t(std::to_integer<unsigned>(b[1]))) << 8)
           | ((std::uint64_t(std::to_integer<unsigned>(b[2]))) << 16)
           | ((std::uint64_t(std::to_integer<unsigned>(b[3]))) << 24)
           | ((std::uint64_t(std::to_integer<unsigned>(b[4]))) << 32)
           | ((std::uint64_t(std::to_integer<unsigned>(b[5]))) << 40)
           | ((std::uint64_t(std::to_integer<unsigned>(b[6]))) << 48)
           | ((std::uint64_t(std::to_integer<unsigned>(b[7]))) << 56);
}
} // namespace detail

// export functions
template <ByteIO IO, class T> void write(IO& out, const T& v);

template <ByteIO IO, class T> void read(IO& in, T& v);

} // namespace tlv
