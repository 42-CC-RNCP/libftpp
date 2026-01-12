// include/utils/endian.hpp
#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace utils
{

// generic endian read/write (C++23 and later)
#if __cplusplus >= 202302L
template <typename T>
inline T read_endian(std::span<const std::byte> bytes, std::endian target)
{
    static_assert(std::is_integral_v<T>, "T must be integral type");

    T value;
    std::memcpy(&value, bytes.data(), sizeof(T));

    if (target != std::endian::native) {
        value = std::byteswap(value);
    }
    return value;
}

template <typename T>
inline T write_endian(T value, std::span<std::byte> bytes, std::endian target)
{
    static_assert(std::is_integral_v<T>, "T must be integral type");

    if (target != std::endian::native) {
        value = std::byteswap(value);
    }
    std::memcpy(bytes.data(), &value, sizeof(T));
    return value;
}

#endif

// little-endian read
inline std::uint32_t read_uint32_le(std::span<const std::byte> bytes)
{
    return (std::uint32_t(std::to_integer<unsigned>(bytes[0])))
           | ((std::uint32_t(std::to_integer<unsigned>(bytes[1]))) << 8)
           | ((std::uint32_t(std::to_integer<unsigned>(bytes[2]))) << 16)
           | ((std::uint32_t(std::to_integer<unsigned>(bytes[3]))) << 24);
}

inline std::uint64_t read_uint64_le(std::span<const std::byte> bytes)
{
    return (std::uint64_t(std::to_integer<unsigned>(bytes[0])))
           | ((std::uint64_t(std::to_integer<unsigned>(bytes[1]))) << 8)
           | ((std::uint64_t(std::to_integer<unsigned>(bytes[2]))) << 16)
           | ((std::uint64_t(std::to_integer<unsigned>(bytes[3]))) << 24)
           | ((std::uint64_t(std::to_integer<unsigned>(bytes[4]))) << 32)
           | ((std::uint64_t(std::to_integer<unsigned>(bytes[5]))) << 40)
           | ((std::uint64_t(std::to_integer<unsigned>(bytes[6]))) << 48)
           | ((std::uint64_t(std::to_integer<unsigned>(bytes[7]))) << 56);
}

// big-endian read
inline std::uint32_t read_uint32_be(std::span<const std::byte> bytes)
{
    return (std::uint32_t(std::to_integer<unsigned>(bytes[0])) << 24)
           | (std::uint32_t(std::to_integer<unsigned>(bytes[1])) << 16)
           | (std::uint32_t(std::to_integer<unsigned>(bytes[2])) << 8)
           | (std::uint32_t(std::to_integer<unsigned>(bytes[3])));
}

inline std::uint64_t read_uint64_be(std::span<const std::byte> bytes)
{
    return (std::uint64_t(std::to_integer<unsigned>(bytes[0])) << 56)
           | (std::uint64_t(std::to_integer<unsigned>(bytes[1])) << 48)
           | (std::uint64_t(std::to_integer<unsigned>(bytes[2])) << 40)
           | (std::uint64_t(std::to_integer<unsigned>(bytes[3])) << 32)
           | (std::uint64_t(std::to_integer<unsigned>(bytes[4])) << 24)
           | (std::uint64_t(std::to_integer<unsigned>(bytes[5])) << 16)
           | (std::uint64_t(std::to_integer<unsigned>(bytes[6])) << 8)
           | (std::uint64_t(std::to_integer<unsigned>(bytes[7])));
}

// little-endian write
inline void write_uint32_le(std::span<std::byte> bytes, std::uint32_t value)
{
    bytes[0] = std::byte(value & 0xFFu);
    bytes[1] = std::byte((value >> 8) & 0xFFu);
    bytes[2] = std::byte((value >> 16) & 0xFFu);
    bytes[3] = std::byte((value >> 24) & 0xFFu);
}

inline void write_uint64_le(std::span<std::byte> bytes, std::uint64_t value)
{
    bytes[0] = std::byte(value & 0xFFu);
    bytes[1] = std::byte((value >> 8) & 0xFFu);
    bytes[2] = std::byte((value >> 16) & 0xFFu);
    bytes[3] = std::byte((value >> 24) & 0xFFu);
    bytes[4] = std::byte((value >> 32) & 0xFFu);
    bytes[5] = std::byte((value >> 40) & 0xFFu);
    bytes[6] = std::byte((value >> 48) & 0xFFu);
    bytes[7] = std::byte((value >> 56) & 0xFFu);
}

// big-endian write
inline void write_uint32_be(std::span<std::byte> bytes, std::uint32_t value)
{
    bytes[0] = std::byte((value >> 24) & 0xFFu);
    bytes[1] = std::byte((value >> 16) & 0xFFu);
    bytes[2] = std::byte((value >> 8) & 0xFFu);
    bytes[3] = std::byte(value & 0xFFu);
}

inline void write_uint64_be(std::span<std::byte> bytes, std::uint64_t value)
{
    bytes[0] = std::byte((value >> 56) & 0xFFu);
    bytes[1] = std::byte((value >> 48) & 0xFFu);
    bytes[2] = std::byte((value >> 40) & 0xFFu);
    bytes[3] = std::byte((value >> 32) & 0xFFu);
    bytes[4] = std::byte((value >> 24) & 0xFFu);
    bytes[5] = std::byte((value >> 16) & 0xFFu);
    bytes[6] = std::byte((value >> 8) & 0xFFu);
    bytes[7] = std::byte(value & 0xFFu);
}

} // namespace utils
