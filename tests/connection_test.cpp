// tests/connection_test.cpp
#include "network/components/connection.hpp"
#include "network/components/message_builder.hpp"
#include "network/contracts/stream_transport.hpp"
#include "network/impl/codec/length_prefixed_codec.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArrayArgument;

class MockStreamTransport : public IStreamTransport
{
public:
    MOCK_METHOD(void, connect, (const Endpoint&), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(ssize_t, recvBytes, (std::byte*, size_t), (override));
    MOCK_METHOD(ssize_t, sendBytes, (const std::byte*, size_t), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(int, nativeHandle, (), (const, override));
};

class ConnectionTest : public ::testing::Test
{
protected:
    std::unique_ptr<MockStreamTransport> transport_;
    std::unique_ptr<LengthPrefixedCodec> codec_;
    std::unique_ptr<Connection> connection_;

    void SetUp() override
    {
        transport_ = std::make_unique<MockStreamTransport>();
        codec_ = std::make_unique<LengthPrefixedCodec>();
        connection_ = std::make_unique<Connection>(*transport_, *codec_);
    }
};

TEST_F(ConnectionTest, InitialStateIsNotClosed)
{
    EXPECT_FALSE(connection_->isClosed());
}

TEST_F(ConnectionTest, InitialStateDoesNotWantWrite)
{
    EXPECT_FALSE(connection_->wantsWrite());
}

TEST_F(ConnectionTest, QueueMessageSetsWantsWrite)
{
    Message msg(100);

    connection_->queue(msg);

    EXPECT_TRUE(connection_->wantsWrite());
}

TEST_F(ConnectionTest, OnReadableWithDataReturnsOk)
{
    std::byte data[10] = {std::byte{0x01},
                          std::byte{0x02},
                          std::byte{0x03},
                          std::byte{0x04},
                          std::byte{0x05},
                          std::byte{0x06},
                          std::byte{0x07},
                          std::byte{0x08},
                          std::byte{0x09},
                          std::byte{0x0A}};

    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(DoAll(SetArrayArgument<0>(data, data + 10), Return(10)));

    auto result = connection_->onReadable();

    EXPECT_EQ(result.status, Connection::IoStatus::Ok);
    EXPECT_EQ(result.bytes, 10u);
}

TEST_F(ConnectionTest, OnReadableWithZeroBytesClosesConnection)
{
    EXPECT_CALL(*transport_, recvBytes(_, _)).WillOnce(Return(0));

    auto result = connection_->onReadable();

    EXPECT_EQ(result.status, Connection::IoStatus::Closed);
    EXPECT_TRUE(connection_->isClosed());
}

TEST_F(ConnectionTest, OnReadableWithErrorReturnsError)
{
    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce([](std::byte*, std::size_t) -> ssize_t {
            errno = EIO;
            return -1;
        });

    auto result = connection_->onReadable();

    EXPECT_EQ(result.status, Connection::IoStatus::Error);
    EXPECT_GT(result.sys_errno, 0);
    EXPECT_TRUE(connection_->isClosed());
}

TEST_F(ConnectionTest, OnWritableWithoutQueuedDataReturnsOk)
{
    auto result = connection_->onWritable();

    EXPECT_EQ(result.status, Connection::IoStatus::Ok);
    EXPECT_EQ(result.bytes, 0u);
}

TEST_F(ConnectionTest, OnWritableWithQueuedDataSendsData)
{
    Message msg(42);
    connection_->queue(msg);

    EXPECT_CALL(*transport_, sendBytes(_, _))
        .WillOnce(Return(8)); // Header size

    auto result = connection_->onWritable();

    EXPECT_EQ(result.status, Connection::IoStatus::Ok);
    EXPECT_EQ(result.bytes, 8u);
}

TEST_F(ConnectionTest, OnWritableWithZeroBytesClosesConnection)
{
    Message msg(42);
    connection_->queue(msg);

    EXPECT_CALL(*transport_, sendBytes(_, _)).WillOnce(Return(0));

    auto result = connection_->onWritable();

    EXPECT_EQ(result.status, Connection::IoStatus::Closed);
    EXPECT_TRUE(connection_->isClosed());
}

TEST_F(ConnectionTest, OnWritableWithErrorReturnsError)
{
    Message msg(42);
    connection_->queue(msg);

    EXPECT_CALL(*transport_, sendBytes(_, _))
        .WillOnce([](const std::byte*, std::size_t) -> ssize_t {
            errno = EPIPE;
            return -1;
        });

    auto result = connection_->onWritable();

    EXPECT_EQ(result.status, Connection::IoStatus::Error);
    EXPECT_GT(result.sys_errno, 0);
    EXPECT_TRUE(connection_->isClosed());
}

TEST_F(ConnectionTest, OnWritableWhenClosedReturnsClosedStatus)
{
    // First close the connection
    EXPECT_CALL(*transport_, recvBytes(_, _)).WillOnce(Return(0));
    connection_->onReadable();

    // Now try to write
    Message msg(1);
    connection_->queue(msg);

    auto result = connection_->onWritable();

    EXPECT_EQ(result.status, Connection::IoStatus::Closed);
}

TEST_F(ConnectionTest, MultipleOnReadableAccumulatesData)
{
    std::byte data1[4] = {
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};
    std::byte data2[4] = {
        std::byte{0x05}, std::byte{0x06}, std::byte{0x07}, std::byte{0x08}};

    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(DoAll(SetArrayArgument<0>(data1, data1 + 4), Return(4)))
        .WillOnce(DoAll(SetArrayArgument<0>(data2, data2 + 4), Return(4)));

    auto result1 = connection_->onReadable();
    auto result2 = connection_->onReadable();

    EXPECT_EQ(result1.bytes, 4u);
    EXPECT_EQ(result2.bytes, 4u);
}

TEST_F(ConnectionTest, PartialSendIsHandledCorrectly)
{
    MessageWriter writer(42);
    int payload = 12345;
    writer << payload;
    Message msg = writer.build();

    connection_->queue(msg);

    // First call sends only 6 bytes out of 12
    EXPECT_CALL(*transport_, sendBytes(_, _))
        .WillOnce(Return(6))
        .WillOnce(Return(6));

    auto result1 = connection_->onWritable();
    EXPECT_EQ(result1.bytes, 6u);
    EXPECT_TRUE(connection_->wantsWrite()); // Still has data to send

    auto result2 = connection_->onWritable();
    EXPECT_EQ(result2.bytes, 6u);
}

TEST_F(ConnectionTest, TryDecodeOnEmptyBufferReturnsNeedMoreData)
{
    Message outMsg(0);
    auto result = connection_->tryDecode(outMsg);

    EXPECT_EQ(result.status, DecodeStatus::NeedMoreData);
}

TEST_F(ConnectionTest, TryDecodeAfterReceivingCompleteMessage)
{
    // Create a complete encoded message
    MessageWriter writer(42);
    int value = 999;
    writer << value;
    Message original = writer.build();

    // Encode it
    DataBufferByteQueue tempQueue;
    codec_->encode(original, tempQueue);

    auto encoded = tempQueue.peek();
    std::vector<std::byte> encodedVec(encoded.begin(), encoded.end());

    // Simulate receiving this data
    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(
            DoAll(SetArrayArgument<0>(encodedVec.data(),
                                      encodedVec.data() + encodedVec.size()),
                  Return(encodedVec.size())));

    connection_->onReadable();

    // Now try to decode
    Message decoded(0);
    auto result = connection_->tryDecode(decoded);

    EXPECT_EQ(result.status, DecodeStatus::Ok);
    EXPECT_EQ(decoded.type(), 42u);

    int decodedValue = 0;
    MessageReader reader(decoded);
    reader >> decodedValue;
    EXPECT_EQ(decodedValue, value);
}

TEST_F(ConnectionTest, ConnectCallsTransportConnect)
{
    Endpoint ep{"127.0.0.1", 8080};

    EXPECT_CALL(*transport_, connect(testing::_));

    connection_->connect(ep);
}

TEST_F(ConnectionTest, MultipleMessagesCanBeQueued)
{
    Message msg1(1);
    Message msg2(2);
    Message msg3(3);

    connection_->queue(msg1);
    connection_->queue(msg2);
    connection_->queue(msg3);

    EXPECT_TRUE(connection_->wantsWrite());
}

TEST_F(ConnectionTest, SendCompleteClearsWantsWrite)
{
    Message msg(42);
    connection_->queue(msg);

    EXPECT_CALL(*transport_, sendBytes(_, _))
        .WillOnce(Return(8)); // Complete send

    connection_->onWritable();

    EXPECT_FALSE(connection_->wantsWrite());
}

TEST_F(ConnectionTest, ReceivePartialMessageThenComplete)
{
    // Create encoded message
    MessageWriter writer(100);
    writer << 123;
    Message original = writer.build();

    DataBufferByteQueue tempQueue;
    codec_->encode(original, tempQueue);
    auto encoded = tempQueue.peek();
    std::vector<std::byte> encodedVec(encoded.begin(), encoded.end());

    // First receive: only 6 bytes
    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(
            DoAll(SetArrayArgument<0>(encodedVec.data(), encodedVec.data() + 6),
                  Return(6)));

    connection_->onReadable();

    Message decoded1(0);
    auto result1 = connection_->tryDecode(decoded1);
    EXPECT_EQ(result1.status, DecodeStatus::NeedMoreData);

    // Second receive: remaining bytes
    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(
            DoAll(SetArrayArgument<0>(encodedVec.data() + 6,
                                      encodedVec.data() + encodedVec.size()),
                  Return(encodedVec.size() - 6)));

    connection_->onReadable();

    Message decoded2(0);
    auto result2 = connection_->tryDecode(decoded2);
    EXPECT_EQ(result2.status, DecodeStatus::Ok);
    EXPECT_EQ(decoded2.type(), 100u);
}
