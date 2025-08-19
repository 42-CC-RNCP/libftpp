#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace tlv
{

enum class WireType : std::uint8_t
{
    VarInt,
    Fixed32,
    Fixed64,
    Bytes,
    NestedTLV
};

template <typename T> struct WireMap
{
    static constexpr WireType value = WireType::NestedTLV;
};

template <> struct WireMap<std::uint32_t>
{
    static constexpr WireType value = WireType::Fixed32;
};

template <> struct WireMap<std::uint64_t>
{
    static constexpr WireType value = WireType::Fixed64;
};

template <> struct WireMap<std::string>
{
    static constexpr WireType value = WireType::Bytes;
};

} // namespace tlv
