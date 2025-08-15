#include "tlv.hpp"
#include <gtest/gtest.h>

using namespace tlv;

TEST(TLV, VarIntBoundary)
{
    struct
    {
        uint64_t v;
        std::size_t expectedLen;
    } tbl[] = {{0, 1}, {127, 1}, {128, 2}, {16'383, 2}, {16'384, 3}};

    for (auto [num, nBytes] : tbl) {
        std::vector<std::byte> buf;
        putVarint(buf, num);
        EXPECT_EQ(buf.size(), nBytes);

        const std::byte* cur = buf.data();
        uint64_t out = getVarint(cur);
        EXPECT_EQ(out, num);
        EXPECT_EQ(cur, buf.data() + nBytes);
    }
}
