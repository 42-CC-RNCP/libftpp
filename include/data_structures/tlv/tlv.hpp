#pragma once
#include "tlv/io.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace tlv
{

// basic wire type
enum class WireType : std::uint8_t
{
    VarInt,
    Fixed32,
    Fixed64,
    Bytes,
    NestedTLV
};

// // serialization traits
// template <class Any, class = void> struct is_serializable : std::false_type
// {
// };

// template <class T>
// struct is_serializable<
//     T,
//     std::void_t<decltype(serialize(std::declval<const T&>(),
//                                    std::declval<auto&>())),
//                 decltype(deserialize(std::declval<auto&>(),
//                                      std::declval<T&>()))> > : std::true_type
// {
// };

// template <class T>
// inline constexpr bool is_serializable_v = is_serializable<T>::value;

// byte-like object can do bulk copy
template <class T>
inline constexpr bool raw_byte_like_v =
    std::is_same_v<std::remove_cv_t<T>, std::byte>
    || std::is_same_v<std::remove_cv_t<T>, unsigned char>
    || std::is_same_v<std::remove_cv_t<T>, std::uint8_t>
    || std::is_same_v<std::remove_cv_t<T>, char>;

// internal functions
namespace detail
{

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
inline void write_fixed32_le(Out& out, std::uint32_t x);
template <ByteWriter Out>
inline void write_fixed64_le(Out& out, std::uint64_t x);
template <ByteReader In> inline std::uint32_t read_fixed32_le(In& in);
template <ByteReader In> inline std::uint64_t read_fixed64_le(In& in);
} // namespace detail

// export functions
template <ByteIO IO, class T> void write(IO& out, const T& v);

template <ByteIO IO, class T> void read(IO& in, T& v);

} // namespace tlv
