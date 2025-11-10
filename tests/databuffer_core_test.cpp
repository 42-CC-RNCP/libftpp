#include <data_structures/data_buffer.hpp>
#include <data_structures/tlv_adapters.hpp>
#include <gtest/gtest.h>
#include <string>
#include <type_traits>

TEST(DataBufferCore, Traits_MoveOnly)
{
    static_assert(!std::is_copy_constructible_v<DataBuffer>);
    static_assert(!std::is_copy_assignable_v<DataBuffer>);
    static_assert(std::is_move_constructible_v<DataBuffer>);
    static_assert(std::is_move_assignable_v<DataBuffer>);
    SUCCEED();
}

TEST(DataBufferCore, Basic_POD_Unsigned_And_Signed)
{
    DataBuffer buf;
    uint64_t u_in = 0x1FFFFu;
    int64_t s_in = -1234567;

    buf << u_in << s_in;

    uint64_t u_out{};
    int64_t s_out{};
    buf >> u_out >> s_out;

    EXPECT_EQ(u_out, u_in);
    EXPECT_EQ(s_out, s_in);
    EXPECT_EQ(buf.remaining(), 0u);
}

TEST(DataBufferCore, Enum_Roundtrip)
{
    enum class Color : unsigned char
    {
        Red = 1,
        Green = 2,
        Blue = 3
    };
    DataBuffer buf;

    Color in = Color::Green;
    buf << in;

    Color out{};
    buf >> out;
    EXPECT_EQ(out, in);
}

TEST(DataBufferCore, Float_Double_Roundtrip)
{
    DataBuffer buf;
    float f_in = 3.1415926f;
    double d_in = -1.5e200;

    buf << f_in << d_in;

    float f_out{};
    double d_out{};
    buf >> f_out >> d_out;

    EXPECT_FLOAT_EQ(f_out, f_in);
    EXPECT_DOUBLE_EQ(d_out, d_in);
}

TEST(DataBufferCore, RawByteLike_Char)
{
    DataBuffer buf;
    char c_in = 'Z';
    buf << c_in;

    char c_out{};
    buf >> c_out;
    EXPECT_EQ(c_out, c_in);
}

TEST(DataBufferCore, String_With_Limits_Write_Check)
{
    DataBuffer buf;
    auto lim = buf.limits();
    lim.max_string_bytes = 4;
    buf.setLimits(lim);

    std::string short_ok = "abcd";
    std::string too_long = "abcde";

    ASSERT_NO_THROW(buf << short_ok);
    EXPECT_THROW(buf << too_long, std::runtime_error);
}

TEST(DataBufferCore, Clear_And_Remaining)
{
    DataBuffer buf;
    buf << uint32_t{42};
    EXPECT_GT(buf.size(), 0u);
    EXPECT_GT(buf.remaining(), 0u);

    uint32_t out{};
    buf >> out;
    EXPECT_EQ(out, 42u);
    EXPECT_EQ(buf.remaining(), 0u);

    buf.clear();
    EXPECT_EQ(buf.size(), 0u);
    EXPECT_EQ(buf.remaining(), 0u);
}
