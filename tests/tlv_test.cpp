#include "tlv/tlv.hpp"
#include <gtest/gtest.h>


TEST(TLV, VarIntBoundary)
{
    struct
    {
        uint64_t v;
        std::size_t expectedLen;
    } tbl[] = {{0, 1}, {127, 1}, {128, 2}, {16'383, 2}, {16'384, 3}};

    for (auto [num, nBytes] : tbl) {
        std::vector<std::byte> buf;
        tlv::Writer::writeVarint(buf, num);
        EXPECT_EQ(buf.size(), nBytes);

        const std::byte* cur = buf.data();
        uint64_t out = tlv::Reader::readVarint(cur);
        EXPECT_EQ(out, num);
        EXPECT_EQ(cur, buf.data() + nBytes);
    }
}

// TEST(TLV, FixedEndianRoundTrip32)
// {
//     std::vector<std::byte> buf;
//     writeFixed<std::uint32_t>(buf, 0x12345678u);
//     const std::byte* p = buf.data();
//     auto v = readFixed<std::uint32_t>(p);
//     EXPECT_EQ(v, 0x12345678u);
// }

// TEST(TLV, FixedEndianRoundTrip64)
// {
//     std::vector<std::byte> buf;
//     writeFixed<std::uint64_t>(buf, 0x1122334455667788ULL);
//     const std::byte* p = buf.data();
//     auto v = readFixed<std::uint64_t>(p);
//     EXPECT_EQ(v, 0x1122334455667788ULL);
// }

// TEST(TLV, BytesBlock)
// {
//     std::vector<std::byte> payload(200);
//     std::iota(payload.begin(), payload.end(), std::byte{0});
//     std::vector<std::byte> buf;
//     writeLenBytes(buf, payload.data(), payload.size());

//     const std::byte* p = buf.data();
//     size_t len{};
//     const std::byte* start = readLenBytes(p, len);
//     ASSERT_EQ(len, payload.size());
//     EXPECT_TRUE(std::equal(start, start+len, payload.begin()));
// }
