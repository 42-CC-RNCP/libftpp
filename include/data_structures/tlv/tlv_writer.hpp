#pragma once
#include "tlv_type.hpp"

namespace tlv
{

class Writer
{
public:
    static void writeVarint(std::vector<std::byte>& buf, std::uint64_t value)
    {
        while (value >= 0x80) {
            buf.push_back(std::byte((value & 0x7F) | 0x80));
            value >>= 7;
        }
        buf.push_back(std::byte(value & 0x7F));
    }
};

} // namespace tlv
