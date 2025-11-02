#include <data_structures/data_buffer.hpp>
#include <data_structures/tlv_adapters.hpp>
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(DataBufferSpecial, Wire_Mismatch_Unsigned_Written_Read_Signed_ShouldThrow)
{
    DataBuffer buf;
    uint32_t u = 42;
    buf << u; // VarUInt
    int32_t s{};
    EXPECT_THROW((buf >> s), std::runtime_error);
}

TEST(DataBufferSpecial, Wire_Mismatch_Fixed32_Read_As_Double_ShouldThrow)
{
    DataBuffer buf;
    float f = 1.5f; // Fixed32
    buf << f;

    double d{};
    EXPECT_THROW((buf >> d), std::runtime_error);
}

TEST(DataBufferSpecial, Varint_Too_Long_ShouldThrow)
{
    DataBuffer buf;

    // header: VarUInt = 0x00
    const std::byte h{std::byte{0x00}};
    buf.writeBytes({&h, 1});
    std::vector<std::byte> seq(11, std::byte{0x80});
    buf.writeBytes({seq.data(), seq.size()});

    uint64_t out{};
    EXPECT_THROW((buf >> out), std::runtime_error);
}

TEST(DataBufferSpecial, RawByteScalar_Size_Mismatch_ShouldThrow)
{
    DataBuffer buf;

    std::string s = "AB";
    buf << s;

    char c{};
    EXPECT_THROW((buf >> c), std::runtime_error);
}

TEST(DataBufferSpecial, String_Max_On_Read_ShouldThrow)
{
    DataBuffer writer;
    std::string s = "abcdef";
    writer << s;

    DataBuffer reader = std::move(writer);
    auto lim = reader.limits();
    lim.max_string_bytes = 5;
    reader.setLimits(lim);

    std::string out;
    EXPECT_THROW((reader >> out), std::runtime_error);
}

TEST(DataBufferSpecial, Remaining_Decreases_And_Underflow_Throws)
{
    DataBuffer buf;
    buf << uint8_t{0x7F};

    uint8_t x{};
    buf >> x;
    EXPECT_EQ(buf.remaining(), 0u);

    uint8_t another{};
    EXPECT_THROW((buf >> another), std::runtime_error);
}
