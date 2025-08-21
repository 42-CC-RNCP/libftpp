#include "tlv.hpp"
#include <bit>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

using namespace tlv;

struct WLWriter
{
    struct Limit
    {
        std::size_t max_message_bytes = (std::size_t)-1;
        std::size_t max_string_bytes = (std::size_t)-1;
        std::size_t max_depth = (std::size_t)-1;
        std::size_t max_elements = (std::size_t)-1;
    } lim{};

    std::vector<std::byte> bytes;
    void writeBytes(std::span<const std::byte> s)
    {
        bytes.insert(bytes.end(), s.begin(), s.end());
    }
    const Limit& limits() const { return lim; }
};

struct MemWriter
{
    std::vector<std::byte> bytes;
    void writeBytes(std::span<const std::byte> s)
    {
        bytes.insert(bytes.end(), s.begin(), s.end());
    }
};

struct MemReader
{
    const std::vector<std::byte>& ref;
    std::size_t pos{0};
    void readExact(std::byte* p, std::size_t n)
    {
        if (pos + n > ref.size()) {
            throw std::runtime_error("underflow");
        }
        std::memcpy(p, ref.data() + pos, n);
        pos += n;
    }
};

// ---------- Header ----------

TEST(TLV_Header, EncodeDecodeRoundTrip)
{
    // write_header -> read_header
    WLWriter w;
    write_header(w, WireType::VarUInt);
    write_header(w, WireType::VarSIntZigZag);
    write_header(w, WireType::Bytes);
    write_header(w, WireType::Fixed32);
    write_header(w, WireType::Fixed64);

    MemReader r{w.bytes};
    EXPECT_EQ(read_header(r), WireType::VarUInt);
    EXPECT_EQ(read_header(r), WireType::VarSIntZigZag);
    EXPECT_EQ(read_header(r), WireType::Bytes);
    EXPECT_EQ(read_header(r), WireType::Fixed32);
    EXPECT_EQ(read_header(r), WireType::Fixed64);
    EXPECT_EQ(r.pos, w.bytes.size());
}

TEST(TLV_Header, UnknownWireThrows)
{
    std::vector<std::byte> bad = {std::byte{0x05}};
    MemReader r{bad};
    EXPECT_THROW((void)read_header(r), std::runtime_error);
}

// ---------- ZigZag ----------

TEST(TLV_ZigZag, KnownMappings32)
{
    EXPECT_EQ(detail::zigzag_encode32(0), 0u);
    EXPECT_EQ(detail::zigzag_encode32(-1), 1u);
    EXPECT_EQ(detail::zigzag_encode32(1), 2u);
    EXPECT_EQ(detail::zigzag_encode32(-2), 3u);
    EXPECT_EQ(detail::zigzag_encode32(std::numeric_limits<int32_t>::max()),
              0xFFFFFFFEu);
    EXPECT_EQ(detail::zigzag_encode32(std::numeric_limits<int32_t>::min()),
              0xFFFFFFFFu);

    EXPECT_EQ(detail::zigzag_decode32(0u), 0);
    EXPECT_EQ(detail::zigzag_decode32(1u), -1);
    EXPECT_EQ(detail::zigzag_decode32(2u), 1);
    EXPECT_EQ(detail::zigzag_decode32(3u), -2);
}

TEST(TLV_ZigZag, KnownMappings64)
{
    EXPECT_EQ(detail::zigzag_encode64(0), 0ull);
    EXPECT_EQ(detail::zigzag_encode64(-1), 1ull);
    EXPECT_EQ(detail::zigzag_encode64(1), 2ull);
    EXPECT_EQ(detail::zigzag_encode64(-2), 3ull);
    EXPECT_EQ(detail::zigzag_decode64(0ull), 0);
    EXPECT_EQ(detail::zigzag_decode64(1ull), -1);
    EXPECT_EQ(detail::zigzag_decode64(2ull), 1);
    EXPECT_EQ(detail::zigzag_decode64(3ull), -2);
}

// ---------- VarUInt ----------

TEST(TLV_VarUInt, KnownVectors)
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
        detail::write_varuint(w, c.v);
        ASSERT_EQ(w.bytes.size(), c.bytes.size());
        for (size_t i = 0; i < c.bytes.size(); ++i) {
            EXPECT_EQ(std::to_integer<std::uint8_t>(w.bytes[i]), c.bytes[i])
                << "i=" << i;
        }

        MemReader r{w.bytes};
        auto out = detail::read_varuint(r);
        EXPECT_EQ(out, c.v);
        EXPECT_EQ(r.pos, c.bytes.size());
    }
}

TEST(TLV_VarUInt, BoundaryLengths)
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
        detail::write_varuint(w, num);
        EXPECT_EQ(w.bytes.size(), nBytes);

        MemReader r{w.bytes};
        uint64_t out = detail::read_varuint(r);
        EXPECT_EQ(out, num);
        EXPECT_EQ(r.pos, nBytes);
    }
}

TEST(TLV_VarIntSigned, KnownVectorsViaZigZag)
{
    struct V
    {
        std::int64_t v;
        std::vector<std::uint8_t> bytes;
    } cases[] = {
        {-1, {0x01}},
        {-2, {0x03}},
        {0, {0x00}},
        {1, {0x02}},
        {63, {0x7E}},
        {64, {0x80, 0x01}}, // zigzag(64)=128 -> 0x80 0x01
    };
    for (auto& c : cases) {
        MemWriter w;
        detail::write_varint_s(w, c.v);
        ASSERT_EQ(w.bytes.size(), c.bytes.size());
        for (size_t i = 0; i < c.bytes.size(); ++i) {
            EXPECT_EQ(std::to_integer<std::uint8_t>(w.bytes[i]), c.bytes[i])
                << "i=" << i;
        }
        MemReader r{w.bytes};
        auto u = detail::read_varuint(r);
        auto s = detail::zigzag_decode64(u);
        EXPECT_EQ(s, c.v);
        EXPECT_EQ(r.pos, w.bytes.size());
    }
}

TEST(TLV_VarUInt, TooLongThrows)
{
    std::vector<std::byte> bad(10, std::byte{0x80});
    MemReader r{bad};
    EXPECT_THROW((void)detail::read_varuint(r), std::runtime_error);
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
    { // fixed32
        std::vector<std::byte> less3 = {
            std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
        MemReader r{less3};
        EXPECT_THROW((void)detail::read_fixed32_le(r), std::runtime_error);
    }
    { // fixed64
        std::vector<std::byte> less7(7, std::byte{0});
        MemReader r{less7};
        EXPECT_THROW((void)detail::read_fixed64_le(r), std::runtime_error);
    }
}

TEST(TLV_Fixed, MixedSequenceRoundTrip)
{
    MemWriter w;
    detail::write_fixed32_le(w, 0x12345678u);
    detail::write_varuint(w, 300);
    detail::write_fixed64_le(w, 0x0102030405060708ull);
    detail::write_varuint(w, 0);

    MemReader r{w.bytes};
    auto a = detail::read_fixed32_le(r);
    auto b = detail::read_varuint(r);
    auto c = detail::read_fixed64_le(r);
    auto d = detail::read_varuint(r);

    EXPECT_EQ(a, 0x12345678u);
    EXPECT_EQ(b, 300u);
    EXPECT_EQ(c, 0x0102030405060708ull);
    EXPECT_EQ(d, 0u);
    EXPECT_EQ(r.pos, w.bytes.size());
}

// ---------- write_value (primitive) ----------

TEST(TLV_WriteValue, UnsignedAndSignedInts)
{
    WLWriter w;

    // u32=300 -> [H=VarUInt(0x00)] [0xAC 0x02]
    {
        uint32_t u = 300;
        write_value(w, u);
        ASSERT_EQ(w.bytes.size(), 1u + 2u);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[0]), 0x00);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[1]), 0xAC);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[2]), 0x02);
        w.bytes.clear();
    }
    // i32=-1 -> [H=VarSIntZigZag(0x01)] [0x01]
    {
        int32_t s = -1;
        write_value(w, s);
        ASSERT_EQ(w.bytes.size(), 2u);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[0]), 0x01);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[1]), 0x01);
        w.bytes.clear();
    }
}

TEST(TLV_WriteValue, FloatAndDouble)
{
    WLWriter w;

    // float 1.0f -> [H=Fixed32(0x03)] [00 00 80 3F]
    {
        float f = 1.0f;
        write_value(w, f);
        ASSERT_EQ(w.bytes.size(), 1u + 4u);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[0]), 0x03);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[1]), 0x00);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[2]), 0x00);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[3]), 0x80);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[4]), 0x3F);
        w.bytes.clear();
    }
    // double 1.0 -> [H=Fixed64(0x04)] [00 00 00 00 00 00 F0 3F]
    {
        double d = 1.0;
        write_value(w, d);
        ASSERT_EQ(w.bytes.size(), 1u + 8u);
        EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[0]), 0x04);
        std::uint8_t expect[8] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F};
        for (int i = 0; i < 8; ++i) {
            EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[1 + i]), expect[i])
                << "i=" << i;
        }
        w.bytes.clear();
    }
}

TEST(TLV_WriteValue, StringAsBytesLenPayload)
{
    WLWriter w;

    std::string s = "hi";
    write_value(w, s);
    ASSERT_EQ(w.bytes.size(), 1u + 1u + 2u); // header + len(2) + 'h''i'
    EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[0]), 0x02); // Bytes
    EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[1]), 0x02); // len=2
    EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[2]), 'h');
    EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[3]), 'i');
}

TEST(TLV_Limits, StringTooLongThrowsOnWrite)
{
    WLWriter w;
    w.lim.max_string_bytes = 1;
    std::string s = "hi"; // len=2

    EXPECT_THROW(write_value(w, s), std::runtime_error);
}

// ---------- write_value (serializable object) ----------

namespace model
{
struct Car
{
    std::uint32_t id;
    std::string model;
};

template <class IO> void serialize(const Car& c, IO& out)
{
    write_value(out, c.id);
    write_value(out, c.model);
}
template <class IO> void deserialize(IO&, Car&) {}
} // namespace model

TEST(TLV_WriteValue, SerializableObjectIsBytesWrapper)
{
    using model::Car;

    Car car{150, "A"};

    WLWriter payload;
    write_value(payload, car.id);    // [00][96 01]
    write_value(payload, car.model); // [02][01]['A']

    // [02][len][payload...]
    WLWriter w;
    write_value(w, car);

    ASSERT_GE(w.bytes.size(), 2u);
    EXPECT_EQ(std::to_integer<uint8_t>(w.bytes[0]), 0x02); // Bytes header

    MemReader r{w.bytes};
    auto wt = read_header(r);
    EXPECT_EQ(wt, WireType::Bytes);
    auto len = detail::read_varuint(r);
    EXPECT_EQ(len, payload.bytes.size());

    ASSERT_EQ(w.bytes.size(), 1 + (r.pos - 1) + payload.bytes.size());
    std::vector<std::byte> tail(w.bytes.begin() + r.pos, w.bytes.end());
    ASSERT_EQ(tail.size(), payload.bytes.size());
    for (size_t i = 0; i < tail.size(); ++i) {
        EXPECT_EQ(std::to_integer<uint8_t>(tail[i]),
                  std::to_integer<uint8_t>(payload.bytes[i]))
            << "i=" << i;
    }
}
