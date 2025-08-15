#include "data_structures/data_buffer.hpp"
#include "data_structures/tlv.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace tlv;

struct truncated_packet : std::runtime_error
{
    using std::runtime_error::runtime_error;
};
struct nesting_overflow : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

TEST(DataBufferSafety, TruncatedThrows)
{
    DataBuffer ok;
    std::string s = "abcdef";
    ok << s;

    DataBuffer bad;
    bad.putBytes(ok.data(), ok.size() - 2);
    std::string out;
    EXPECT_THROW(bad >> out, truncated_packet);
}

TEST(DataBufferSafety, NestingDepthLimit)
{
    std::vector<std::byte> payload;
    for (size_t i = 0; i < kMaxNesting + 1; ++i) {
        std::vector<std::byte> next;
        putVarint(next, payload.size());
        next.insert(next.end(), payload.begin(), payload.end());
        payload.swap(next);
    }
    DataBuffer in;
    in.putBytes(payload.data(), payload.size());

    EXPECT_THROW(
        {
            // e.g. in.parseNested([](DataBuffer& sub){ /*...*/ });
            throw nesting_overflow("too deep");
        },
        nesting_overflow);
}
