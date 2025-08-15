// tests/databuffer_core_test.cpp
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include "data_structures/data_buffer.hpp"
#include "data_structures/tlv.hpp"
#include "data_structures/serialization_traits.hpp"

using namespace tlv;

TEST(DataBufferCore, PrimitiveRoundTrip)
{
    DataBuffer buf;
    std::int32_t a = 42;
    std::uint64_t b = 0x1122334455667788ULL;
    double c = 3.141592653589793;

    buf << a << b << c;

    DataBuffer in;
    in.putBytes(buf.data(), buf.size());

    std::int32_t a2{}; std::uint64_t b2{}; double c2{};
    in >> a2 >> b2 >> c2;

    EXPECT_EQ(a2, a);
    EXPECT_EQ(b2, b);
    EXPECT_DOUBLE_EQ(c2, c);
}

TEST(DataBufferCore, StringRoundTrip)
{
    DataBuffer buf;
    std::string s = "hello world";
    buf << s;

    DataBuffer in;
    in.putBytes(buf.data(), buf.size());

    std::string t;
    in >> t;
    EXPECT_EQ(t, s);
}

TEST(DataBufferCore, StreamIOFrame)
{
    DataBuffer buf;
    std::string msg = "frame";
    buf << msg;

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    ss << buf;                // DataBuffer → stream
    DataBuffer in;
    ss.seekg(0);
    ss >> in;                 // stream → DataBuffer

    std::string out;
    in >> out;
    EXPECT_EQ(out, msg);
}
