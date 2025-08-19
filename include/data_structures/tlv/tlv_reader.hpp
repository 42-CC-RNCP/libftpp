#pragma once
#include "tlv_type.hpp"

namespace tlv
{

class Reader
{
public:
    static std::uint64_t readVarint(const std::byte*& p)
    {
        std::uint64_t value = 0;
        int shift = 0;

        while (true) {
            std::uint8_t b = std::uint8_t(*p++);
            value |= std::uint64_t(b & 0x7F) << shift;
            if (!(b & 0x80)) {
                break;
            }
            shift += 7;
        }

        return value;
    }
};

} // namespace tlv
