#pragma once
#include "tlv_io.hpp"
#include "tlv_type_traits.hpp"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>

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
inline void write_varuint(Out& out, std::uint64_t value)
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

inline std::size_t varuint_len(std::uint64_t n)
{
    Sizer s;
    tlv::detail::write_varuint(s, n);
    return s.n;
}

// ZigZag + VarInt for signed
template <class Out> inline void write_varint_s(Out&& out, std::int64_t n)
{
    write_varuint(out, zigzag_encode64(n));
}

template <ByteReader In> inline std::uint64_t read_varuint(In& in)
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

template <ByteReader In> inline std::int64_t read_varint_s(In& in)
{
    const std::uint64_t n = read_varuint(in);
    return zigzag_decode64(n);
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

template <ByteWriter Out> void write_fixed64_le(Out& out, std::uint64_t x)
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
template <ByteWriter IO, class T> inline void write_value(IO& out, const T& v)
{
    static_assert(!std::is_pointer_v<T>,
                  "TODO: pointers are not serializable.");
    // 1. handle raw bytes
    // 2. handle int and enum
    // 2.1 -> unsigned int with varint to save space
    // 2.2 -> signed int with zigzag or Fixed*
    // 3. handle float
    // 4. handle string
    // 5.1 -> std container: vector
    // 5.2 -> std container: array
    // 6. handle void
    // 7. handle optional
    // 8. handle serializable
    // handle others

    if constexpr (raw_byte_like_v<T>) {
        // header
        write_header(out, WireType::Bytes);
        // len
        constexpr std::uint64_t n = sizeof(T);
        detail::write_varuint(out, n);
        // payload
        // convert `1` of object T which continus in mem to bytes
        out.writeBytes(std::as_bytes(std::span{&v, 1}));
    }
    else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
        // extract real type if it is enum
        using U = typename std::conditional_t<std::is_enum_v<T>,
                                              std::underlying_type<T>,
                                              std::type_identity<T>>::type;

        if constexpr (std::is_signed_v<U>) {
            write_header(out, WireType::VarSIntZigZag);
            detail::write_varint_s(out, static_cast<std::int64_t>(v));
        }
        else {
            write_header(out, WireType::VarUInt);
            detail::write_varuint(out, static_cast<std::uint64_t>(v));
        }
    }
    else if constexpr (std::is_floating_point_v<T>) {
        if constexpr (sizeof(T) == 4) {
            write_header(out, WireType::Fixed32);
            detail::write_fixed32_le(out, std::bit_cast<std::uint32_t>(v));
        }
        else {
            write_header(out, WireType::Fixed64);
            detail::write_fixed64_le(out, std::bit_cast<std::uint64_t>(v));
        }
    }
    else if constexpr (is_container_like<T>) {
        Sizer s;
        std::uint64_t elem_count = 0;

        if constexpr (requires(const T& x) { x.size(); }) {
            elem_count = static_cast<std::uint64_t>(v.size());
        }
        else {
            elem_count = static_cast<std::uint64_t>(
                std::distance(std::begin(v), std::end(v)));
        }

        // element count also part of the payload
        s.n += detail::varuint_len(elem_count);

        // iter each element in the sequence container to sum up the total
        // payload size
        for (auto const& e : v) {
            write_value(s, e);
        }
        // header
        write_header(out, WireType::Bytes);
        // Bytes(len)
        detail::write_varuint(out, s.n);
        // payload: count of element
        detail::write_varuint(out, elem_count);
        // payload: contain of each element
        for (auto const& e : v) {
            write_value(out, e);
        }
    }
    else if constexpr (std::is_same_v<std::remove_cv_t<T>, std::string>) {
        // header
        write_header(out, WireType::Bytes);
        auto n = v.size();

        if constexpr (requires { out.limits().max_string_bytes; }) {
            if (n > out.limits().max_string_bytes) {
                throw std::runtime_error("string too long");
            }
        }
        // len
        detail::write_varuint(out, n);
        // payload
        if (n) {
            out.writeBytes(std::as_bytes(std::span{v.data(), n}));
        }
    }
    else if constexpr (serializable_v<T, IO>) {
        Sizer s;

        // 1st pass to get the total length of all the fields
        serialize(v, s);
        // header
        write_header(out, WireType::Bytes);
        // write len
        detail::write_varuint(out, s.n);
        // 2nd pass write payload
        serialize(v, out);
    }
    else {
        static_assert(dependent_false_v<T>,
                      "Type not serializable; provide serialize() or TBD");
    }
}

template <ByteReader IO, class T> inline void read_value(IO& in, T& v)
{
    static_assert(!std::is_pointer_v<T>,
                  "TODO: pointers are not deserializable.");

    const WireType wt = read_header(in);

    switch (wt) {
    case WireType::VarUInt: {
        auto u = detail::read_varuint(in);

        if constexpr (std::is_integral_v<T> && !std::is_signed_v<T>) {
            v = static_cast<T>(u);
        }
        else if constexpr (std::is_enum_v<T>) {
            if constexpr (!std::is_signed_v<std::underlying_type_t<T>>) {
                v = static_cast<T>(u);
            }
            else {
                throw std::runtime_error(
                    "miss matched decode failed: VarUInt (enum with signed underlying)");
            }
        }
        else {
            throw std::runtime_error("miss matched decode failed: VarUInt");
        }
        break;
    }
    case WireType::VarSIntZigZag: {
        auto s = detail::read_varint_s(in);
        if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
            v = static_cast<T>(s);
        }
        else if constexpr (std::is_enum_v<T>) {
            if constexpr (std::is_signed_v<std::underlying_type_t<T>>) {
                v = static_cast<T>(s);
            }
            else {
                throw std::runtime_error(
                    "miss matched decode failed: VarSIntZigZag (enum with signed underlying)");
            }
        }
        else {
            throw std::runtime_error(
                "miss matched decode failed: VarSIntZigZag");
        }
        break;
    }
    case WireType::Fixed32: {
        auto x = detail::read_fixed32_le(in);
        if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4) {
            v = std::bit_cast<T>(x);
        }
        else if constexpr (std::is_integral_v<T> && sizeof(T) <= 4) {
            v = static_cast<T>(x);
        }
        else {
            throw std::runtime_error("miss matched decode failed: Fixed32");
        }
        break;
    }
    case WireType::Fixed64: {
        auto x = detail::read_fixed64_le(in);
        if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8) {
            v = std::bit_cast<T>(x);
        }
        else if constexpr (std::is_integral_v<T> && sizeof(T) <= 8) {
            v = static_cast<T>(x);
        }
        else {
            throw std::runtime_error("miss matched decode failed: Fixed64");
        }
        break;
    }
    case WireType::Bytes: {
        size_t len = detail::read_varuint(in);

        if (!len) {
            // do nothing
            break;
        }

        // 1. raw bytes
        // 2. string
        // 3. serialized
        if constexpr (raw_byte_like_v<T>) {
            if (len != sizeof(T)) {
                throw std::runtime_error("raw byte scalar size mismatch");
            }
            in.readExact(reinterpret_cast<std::byte*>(&v), sizeof(T));
        }
        else if constexpr (std::is_same_v<std::string, std::remove_cv_t<T>>) {
            if constexpr (requires { in.limits().max_string_bytes; }) {
                if (len > in.limits().max_string_bytes) {
                    throw std::runtime_error("string too long");
                }
            }
            v.resize(len);
            in.readExact(reinterpret_cast<std::byte*>(v.data()), len);
        }
        else if constexpr (is_container_like<T>) {
            std::uint64_t elem_count = detail::read_varuint(in);

            if constexpr (requires { in.limits().max_elements; }) {
                if (elem_count > in.limits().max_elements) {
                    throw std::runtime_error("too many elements");
                }
            }

            if constexpr (is_std_array_v<T>) {
                constexpr std::uint64_t N =
                    static_cast<std::uint64_t>(std::tuple_size_v<T>);
                if (elem_count != N) {
                    throw std::runtime_error(
                        "read_value: std::array size mismatch");
                }
                for (std::size_t i = 0; i < N; ++i) {
                    read_value(in, v[i]);
                }
            }
            else {
                if constexpr (requires(T& x) { x.clear(); }) {
                    v.clear();
                }

                auto it = std::inserter(v, std::end(v));
                for (std::uint64_t i = 0; i < elem_count; ++i) {
                    typename T::value_type elem{};
                    read_value(in, elem);
                    *it++ = std::move(elem);
                }
            }
        }
        else if constexpr (serializable_v<T, IO>) {
            deserialize(in, v);
        }
        else {
            throw std::runtime_error("miss matched decode failed: Bytes");
        }
        break;
    }
    default:
        throw std::runtime_error("cannot match to any type");
    }
}

} // namespace tlv
