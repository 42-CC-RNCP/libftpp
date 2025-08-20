#include "tlv.hpp"
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>

using namespace tlv;

struct MemWriter
{
    std::vector<std::byte> bytes;
    void writeBytes(std::span<const std::byte> s)
    { // ByteWriter
        bytes.insert(bytes.end(), s.begin(), s.end());
    }
};

struct MemReader
{
    const std::vector<std::byte>& ref;
    std::size_t pos{0};
    void readExact(std::byte* p, std::size_t n)
    { // ByteReader
        if (pos + n > ref.size()) {
            throw std::runtime_error("underflow");
        }
        std::memcpy(p, ref.data() + pos, n);
        pos += n;
    }
};

// ---------- VarInt ----------

TEST(TLV_VarInt, KnownVectors)
{
    struct V
    {
        std::uint64_t v;
        std::vector<std::uint8_t> bytes;
    } cases[] = {
        {0ull, {0x00}},
        {1ull, {0x01}},
        {127ull, {0x7F}},
        {128ull, {0x80, 0x01}},
        {300ull, {0xAC, 0x02}},
        {16383ull, {0xFF, 0x7F}},
        {16384ull, {0x80, 0x80, 0x01}},
        {0xFFFFFFFFull, {0xFF, 0xFF, 0xFF, 0xFF, 0x0F}},
        {0xFFFFFFFFFFFFFFFFull,
         {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}},
    };

    for (auto& c : cases) {
        MemWriter w;
        detail::write_varint(w, c.v);
        ASSERT_EQ(w.bytes.size(), c.bytes.size());

        for (size_t i = 0; i < c.bytes.size(); ++i) {
            EXPECT_EQ(std::to_integer<std::uint8_t>(w.bytes[i]), c.bytes[i])
                << "i=" << i;
        }

        MemReader r{w.bytes};
        auto out = detail::read_varint(r);
        EXPECT_EQ(out, c.v);
        EXPECT_EQ(r.pos, c.bytes.size());
    }
}

TEST(TLV_VarInt, BoundaryLengths)
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
        detail::write_varint(w, num);
        EXPECT_EQ(w.bytes.size(), nBytes);

        MemReader r{w.bytes};
        uint64_t out = detail::read_varint(r);
        EXPECT_EQ(out, num);
        EXPECT_EQ(r.pos, nBytes);
    }
}

TEST(TLV_VarInt, TooLongThrows)
{
    // 10 bytes all with continuation bit set (no terminator)
    std::vector<std::byte> bad(10, std::byte{0x80});
    MemReader r{bad};
    EXPECT_THROW((void)detail::read_varint(r), std::runtime_error);
}

TEST(TLV_VarInt, SequentialReadWrite)
{
    std::vector<std::uint64_t> nums = {0,
                                       1,
                                       127,
                                       128,
                                       255,
                                       256,
                                       300,
                                       16383,
                                       16384,
                                       0x12345678ull,
                                       0xFFFFFFFFull,
                                       0xABCDEF0123456789ull};

    MemWriter w;
    for (auto v : nums) {
        detail::write_varint(w, v);
    }

    MemReader r{w.bytes};
    for (auto expect : nums) {
        auto got = detail::read_varint(r);
        EXPECT_EQ(got, expect);
    }
    EXPECT_EQ(r.pos, w.bytes.size());
}

// ---------- Fixed LE ----------

TEST(TLV_Fixed, Fixed32_EncodingMatchesLE)
{
    struct C
    {
        std::uint32_t x;
        std::vector<std::uint8_t> bex;
    } cs[] = {
        {0x00000000u, {0x00, 0x00, 0x00, 0x00}},
        {0x00000001u, {0x01, 0x00, 0x00, 0x00}},
        {0x12345678u, {0x78, 0x56, 0x34, 0x12}},
        {0xFFFFFFFFu, {0xFF, 0xFF, 0xFF, 0xFF}},
    };
    for (auto& c : cs) {
        MemWriter w;
        detail::write_fixed32_le(w, c.x);
        ASSERT_EQ(w.bytes.size(), 4u);
        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ(std::to_integer<std::uint8_t>(w.bytes[i]), c.bex[i])
                << "i=" << i;
        }

        MemReader r{w.bytes};
        auto got = detail::read_fixed32_le(r);
        EXPECT_EQ(got, c.x);
        EXPECT_EQ(r.pos, 4u);
    }
}

TEST(TLV_Fixed, Fixed64_EncodingMatchesLE)
{
    struct C
    {
        std::uint64_t x;
        std::vector<std::uint8_t> bex;
    } cs[] = {
        {0x0000000000000000ull, {0, 0, 0, 0, 0, 0, 0, 0}},
        {0x0000000000000001ull, {1, 0, 0, 0, 0, 0, 0, 0}},
        {0x0102030405060708ull,
         {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01}},
        {0xFFFFFFFFFFFFFFFFull,
         {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
    };
    for (auto& c : cs) {
        MemWriter w;
        detail::write_fixed64_le(w, c.x);
        ASSERT_EQ(w.bytes.size(), 8u);
        for (int i = 0; i < 8; ++i) {
            EXPECT_EQ(std::to_integer<std::uint8_t>(w.bytes[i]), c.bex[i])
                << "i=" << i;
        }

        MemReader r{w.bytes};
        auto got = detail::read_fixed64_le(r);
        EXPECT_EQ(got, c.x);
        EXPECT_EQ(r.pos, 8u);
    }
}

TEST(TLV_Fixed, UnderflowThrows)
{
    // fixed32
    {
        std::vector<std::byte> less3 = {
            std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
        MemReader r{less3};
        EXPECT_THROW((void)detail::read_fixed32_le(r), std::runtime_error);
    }
    // fixed64
    {
        std::vector<std::byte> less7(7, std::byte{0});
        MemReader r{less7};
        EXPECT_THROW((void)detail::read_fixed64_le(r), std::runtime_error);
    }
}

TEST(TLV_Fixed, MixedSequenceRoundTrip)
{
    MemWriter w;
    detail::write_fixed32_le(w, 0x12345678u);
    detail::write_varint(w, 300);
    detail::write_fixed64_le(w, 0x0102030405060708ull);
    detail::write_varint(w, 0);

    MemReader r{w.bytes};
    auto a = detail::read_fixed32_le(r);
    auto b = detail::read_varint(r);
    auto c = detail::read_fixed64_le(r);
    auto d = detail::read_varint(r);

    EXPECT_EQ(a, 0x12345678u);
    EXPECT_EQ(b, 300u);
    EXPECT_EQ(c, 0x0102030405060708ull);
    EXPECT_EQ(d, 0u);
    EXPECT_EQ(r.pos, w.bytes.size());
}
