// tests/length_prefixed_codec_test.cpp
#include "network/core/message.hpp"
#include "network/impl/buffer/byte_queue_adapter.hpp"
#include "network/impl/codec/length_prefixed_codec.hpp"
#include <gtest/gtest.h>
#include <vector>

TEST(LengthPrefixedCodecTest, EncodeMessageWithoutPayload)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;
    Message msg(100);

    codec.encode(msg, queue);

    // Length (4 bytes) + Type (4 bytes) = 8 bytes
    EXPECT_EQ(queue.remaining(), 8u);
}

TEST(LengthPrefixedCodecTest, EncodeMessageWithPayload)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;
    Message msg(200);

    int payload = 12345;
    msg << payload;

    codec.encode(msg, queue);

    // Length (4) + Type (4) + int (4) = 12 bytes
    EXPECT_EQ(queue.remaining(), 12u);
}

TEST(LengthPrefixedCodecTest, EncodeMessageWithStringPayload)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;
    Message msg(300);

    std::string str = "hello";
    msg << str;

    codec.encode(msg, queue);

    // Length (4) + Type (4) + string data
    EXPECT_GT(queue.remaining(), 8u);
}

TEST(LengthPrefixedCodecTest, DecodeReturnsNeedMoreDataForPartialHeader)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    // Only 6 bytes, need 8 for full header
    std::vector<std::byte> partial{std::byte{0x00},
                                   std::byte{0x00},
                                   std::byte{0x00},
                                   std::byte{0x08},
                                   std::byte{0x00},
                                   std::byte{0x00}};
    queue.append(partial);

    Message outMsg(0);
    auto result = codec.tryDecode(queue, outMsg);

    EXPECT_EQ(result.status, DecodeStatus::NeedMoreData);
}

TEST(LengthPrefixedCodecTest, DecodeReturnsNeedMoreDataForIncompletePayload)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    // Header says 10 bytes total (type + payload), but only provide header
    std::vector<std::byte> incomplete{
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x0A}, // len=10
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x2A} // type=42
        // Missing 6 bytes of payload
    };
    queue.append(incomplete);

    Message outMsg(0);
    auto result = codec.tryDecode(queue, outMsg);

    EXPECT_EQ(result.status, DecodeStatus::Invalid);
}

TEST(LengthPrefixedCodecTest, DecodeReturnsInvalidForBadLength)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    // Length = 2, which is less than type size (4)
    std::vector<std::byte> invalid{std::byte{0x00},
                                   std::byte{0x00},
                                   std::byte{0x00},
                                   std::byte{0x02},
                                   std::byte{0x00},
                                   std::byte{0x00},
                                   std::byte{0x00},
                                   std::byte{0x01}};
    queue.append(invalid);

    Message outMsg(0);
    auto result = codec.tryDecode(queue, outMsg);

    EXPECT_EQ(result.status, DecodeStatus::Invalid);
}

TEST(LengthPrefixedCodecTest, EncodeDecodeRoundTripWithoutPayload)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    Message original(42);
    codec.encode(original, queue);

    Message decoded(0);
    auto result = codec.tryDecode(queue, decoded);

    EXPECT_EQ(result.status, DecodeStatus::Ok);
    EXPECT_EQ(decoded.type(), 42u);
    EXPECT_EQ(queue.remaining(), 0u);
}

TEST(LengthPrefixedCodecTest, EncodeDecodeRoundTripWithIntPayload)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    Message original(100);
    int value = 987654321;
    original << value;

    codec.encode(original, queue);

    Message decoded(0);
    auto result = codec.tryDecode(queue, decoded);

    EXPECT_EQ(result.status, DecodeStatus::Ok);
    EXPECT_EQ(decoded.type(), 100u);

    int decodedValue = 0;
    decoded >> decodedValue;
    EXPECT_EQ(decodedValue, value);
}

TEST(LengthPrefixedCodecTest, EncodeDecodeRoundTripWithStringPayload)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    Message original(200);
    std::string value = "hello, world";
    original << value;

    codec.encode(original, queue);

    Message decoded(0);
    auto result = codec.tryDecode(queue, decoded);

    EXPECT_EQ(result.status, DecodeStatus::Ok);
    EXPECT_EQ(decoded.type(), 200u);

    std::string decodedValue;
    decoded >> decodedValue;
    EXPECT_EQ(decodedValue, value);
}

TEST(LengthPrefixedCodecTest, EncodeDecodeMultipleFields)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    Message original(300);
    int i = 42;
    std::string s = "test";
    double d = 3.14;
    original << i << s << d;

    codec.encode(original, queue);

    Message decoded(0);
    auto result = codec.tryDecode(queue, decoded);

    EXPECT_EQ(result.status, DecodeStatus::Ok);

    int i_out = 0;
    std::string s_out;
    double d_out = 0.0;
    decoded >> i_out >> s_out >> d_out;

    EXPECT_EQ(i_out, i);
    EXPECT_EQ(s_out, s);
    EXPECT_DOUBLE_EQ(d_out, d);
}

TEST(LengthPrefixedCodecTest, DecodeMultipleMessagesInSequence)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    // Encode two messages
    Message msg1(1);
    int val1 = 111;
    msg1 << val1;
    codec.encode(msg1, queue);

    Message msg2(2);
    int val2 = 222;
    msg2 << val2;
    codec.encode(msg2, queue);

    // Decode first message
    Message decoded1(0);
    auto result1 = codec.tryDecode(queue, decoded1);
    EXPECT_EQ(result1.status, DecodeStatus::Ok);
    EXPECT_EQ(decoded1.type(), 1u);

    int out1 = 0;
    decoded1 >> out1;
    EXPECT_EQ(out1, val1);

    // Decode second message
    Message decoded2(0);
    auto result2 = codec.tryDecode(queue, decoded2);
    EXPECT_EQ(result2.status, DecodeStatus::Ok);
    EXPECT_EQ(decoded2.type(), 2u);

    int out2 = 0;
    decoded2 >> out2;
    EXPECT_EQ(out2, val2);

    // Queue should be empty
    EXPECT_EQ(queue.remaining(), 0u);
}

TEST(LengthPrefixedCodecTest, DecodeWithExtraDataInQueue)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    // Encode one complete message
    Message msg1(10);
    msg1 << 100;
    codec.encode(msg1, queue);

    // Add partial second message
    std::vector<std::byte> partial{
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x08}};
    queue.append(partial);

    // Should successfully decode first message
    Message decoded(0);
    auto result = codec.tryDecode(queue, decoded);

    EXPECT_EQ(result.status, DecodeStatus::Ok);
    EXPECT_EQ(decoded.type(), 10u);

    // Remaining data should be in queue
    EXPECT_GT(queue.remaining(), 0u);
}

TEST(LengthPrefixedCodecTest, LargePayloadEncodeDecode)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    Message original(999);
    std::string largeStr(5000, 'X');
    original << largeStr;

    codec.encode(original, queue);

    Message decoded(0);
    auto result = codec.tryDecode(queue, decoded);

    EXPECT_EQ(result.status, DecodeStatus::Ok);
    EXPECT_EQ(decoded.type(), 999u);

    std::string decodedStr;
    decoded >> decodedStr;
    EXPECT_EQ(decodedStr.size(), 5000u);
    EXPECT_EQ(decodedStr, largeStr);
}

TEST(LengthPrefixedCodecTest, EmptyQueueDecodeReturnsNeedMoreData)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    Message outMsg(0);
    auto result = codec.tryDecode(queue, outMsg);

    EXPECT_EQ(result.status, DecodeStatus::NeedMoreData);
}

TEST(LengthPrefixedCodecTest, ConsumesExactBytesOnSuccessfulDecode)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    Message original(50);
    codec.encode(original, queue);

    std::size_t beforeDecode = queue.remaining();

    Message decoded(0);
    codec.tryDecode(queue, decoded);

    EXPECT_EQ(queue.remaining(), 0u);
    EXPECT_EQ(beforeDecode, 8u); // header only
}

TEST(LengthPrefixedCodecTest, PartialDecodeDoesNotConsumeData)
{
    LengthPrefixedCodec codec;
    DataBufferByteQueue queue;

    std::vector<std::byte> partial{std::byte{0x00}, std::byte{0x00}};
    queue.append(partial);

    std::size_t before = queue.remaining();

    Message outMsg(0);
    codec.tryDecode(queue, outMsg);

    // Should not consume anything
    EXPECT_EQ(queue.remaining(), before);
}
