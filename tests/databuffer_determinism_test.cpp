#include <gtest/gtest.h>
#include <vector>
#include "data_structures/data_buffer.hpp"
#include "data_structures/tlv.hpp"
#include "data_structures/schema.hpp"

using namespace tlv;

struct Mini { int x; std::string s; };

static std::vector<std::byte> ExpectedBytes()
{
    std::vector<std::byte> out;
    // Tag=1, x=150 (VarInt)
    putVarint(out, 1);
    putVarint(out, 150);
    // Tag=2, s="OK" (Bytes)
    putVarint(out, 2);
    putVarint(out, 2); // len
    out.push_back(std::byte('O'));
    out.push_back(std::byte('K'));
    return out;
}

TEST(DataBufferDeterminism, CanonicalEncoding)
{
    register_schema<Mini>( field<1>(&Mini::x), field<2>(&Mini::s) );
    Mini m{150, "OK"};

    DataBuffer buf; buf << m;
    auto expect = ExpectedBytes();

    ASSERT_EQ(buf.size(), expect.size());
    EXPECT_TRUE(std::equal(buf.data(), buf.data()+buf.size(), expect.begin()));
}
