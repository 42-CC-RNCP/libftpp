#include "tlv/tlv.hpp"
#include <gtest/gtest.h>

struct MemWriter
{
    std::vector<std::byte> bytes;
    void writeBytes(std::span<const std::byte> s)
    { // ByteWriter concept
        bytes.insert(bytes.end(), s.begin(), s.end());
    }
};
struct MemReader
{
    const std::vector<std::byte>& ref;
    std::size_t pos{0};
    void readExact(std::byte* p, std::size_t n)
    { // ByteReader concept
        if (pos + n > ref.size()) {
            throw std::runtime_error("underflow");
        }
        std::memcpy(p, ref.data() + pos, n);
        pos += n;
    }
};

TEST(TLV, VarIntBoundary)
{
    struct Case
    {
        uint64_t v;
        std::size_t expectedLen;
    } tbl[] = {{0ull, 1},
               {127ull, 1},
               {128ull, 2},
               {16383ull, 2},
               {16384ull, 3},
               {0xFFFFFFFFull, 5},
               {0xFFFFFFFFFFFFFFFFull, 10}};

    for (auto [num, nBytes] : tbl) {
        MemWriter w;
        tlv::detail::write_varint(w, num); // encode
        EXPECT_EQ(w.bytes.size(), nBytes);

        MemReader r{w.bytes};
        uint64_t out = tlv::detail::read_varint(r); // decode
        EXPECT_EQ(out, num);
        EXPECT_EQ(r.pos, nBytes);
    }
}

TEST(TLV, VarIntTooLongThrows)
{
    std::vector<std::byte> bad(10, std::byte{static_cast<unsigned char>(0x80)});
    MemReader r{bad};
    EXPECT_THROW({ (void)tlv::detail::read_varint(r); }, std::runtime_error);
}
