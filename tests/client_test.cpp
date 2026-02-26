// tests/client_test.cpp
#include "network/components/client.hpp"
#include "network/contracts/stream_transport.hpp"
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
};

class MockMessageCodec : public IMessageCodec
{
public:
    MOCK_METHOD(void, encode, (const Message&, ByteQueue&), (override));
    MOCK_METHOD(DecodeResult, tryDecode, (ByteQueue&, Message&), (override));
};

class ClientTest : public ::testing::Test
{
protected:
    std::unique_ptr<MockStreamTransport> transport_;
    std::unique_ptr<MockMessageCodec> codec_;
    std::unique_ptr<Client> client_;

    void SetUp() override
    {
        transport_ = std::make_unique<MockStreamTransport>();
        codec_ = std::make_unique<MockMessageCodec>();
        client_ = std::make_unique<Client>(*transport_, *codec_);
    }
};

TEST_F(ClientTest, ConnectCallsTransportConnect)
{
    std::string addr = "127.0.0.1";
    std::size_t port = 8080;

    EXPECT_CALL(*transport_, connect(_));

    client_->connect(addr, port);
}

TEST_F(ClientTest, SendEncodesMessage)
{
    Message msg(100);

    EXPECT_CALL(*codec_, encode(_, _));

    client_->send(msg);
}

TEST_F(ClientTest, UpdateReadsFromTransport)
{
    std::byte data[10] = {std::byte{0x01}};

    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(DoAll(SetArrayArgument<0>(data, data + 10), Return(10)));

    EXPECT_CALL(*codec_, tryDecode(_, _))
        .WillOnce(Return(DecodeResult{DecodeStatus::NeedMoreData, 0, ""}));

    client_->update();
}

TEST_F(ClientTest, UpdateWithClosedConnectionReturns)
{
    EXPECT_CALL(*transport_, recvBytes(_, _)).WillOnce(Return(0));

    EXPECT_CALL(*transport_, disconnect());

    // Should not throw
    client_->update();
}

TEST_F(ClientTest, UpdateWithIOErrorThrows)
{
    EXPECT_CALL(*transport_, recvBytes(_, _)).WillOnce(Return(-1));

    EXPECT_THROW(client_->update(), std::runtime_error);
}

TEST_F(ClientTest, UpdateDispatchesDecodedMessages)
{
    Message::Type handlerType = 42;
    bool handlerCalled = false;

    ClientHandler handler = [&](OutboundPort&, const Message&) {
        handlerCalled = true;
    };

    client_->defineAction(handlerType, handler);

    // Simulate successful read
    std::byte data[10] = {std::byte{0x01}};
    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(DoAll(SetArrayArgument<0>(data, data + 10), Return(10)));

    // Simulate successful decode
    EXPECT_CALL(*codec_, tryDecode(_, _))
        .WillOnce([handlerType](ByteQueue&, Message& outMsg) {
            outMsg.type() = handlerType;
            return DecodeResult{DecodeStatus::Ok, 0, ""};
        })
        .WillRepeatedly(
            Return(DecodeResult{DecodeStatus::NeedMoreData, 0, ""}));

    client_->update();

    EXPECT_TRUE(handlerCalled);
}

TEST_F(ClientTest, UpdateWithDecodeErrorThrows)
{
    std::byte data[10] = {std::byte{0x01}};

    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(DoAll(SetArrayArgument<0>(data, data + 10), Return(10)));

    EXPECT_CALL(*codec_, tryDecode(_, _))
        .WillOnce(
            Return(DecodeResult{DecodeStatus::Invalid, -1, "Bad format"}));

    EXPECT_THROW(client_->update(), std::runtime_error);
}

TEST_F(ClientTest, UpdateDecodesSingleMessage)
{
    Message::Type msgType = 100;
    int callCount = 0;

    ClientHandler handler = [&](OutboundPort&, const Message&) {
        callCount++;
    };

    client_->defineAction(msgType, handler);

    std::byte data[10] = {std::byte{0x01}};

    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(DoAll(SetArrayArgument<0>(data, data + 10), Return(10)));

    EXPECT_CALL(*codec_, tryDecode(_, _))
        .WillOnce([msgType](ByteQueue&, Message& outMsg) {
            outMsg.type() = msgType;
            return DecodeResult{DecodeStatus::Ok, 0, ""};
        })
        .WillOnce(Return(DecodeResult{DecodeStatus::NeedMoreData, 0, ""}));

    client_->update();

    EXPECT_EQ(callCount, 1);
}

TEST_F(ClientTest, UpdateDecodesMultipleMessages)
{
    Message::Type msgType = 200;
    int callCount = 0;

    ClientHandler handler = [&](OutboundPort&, const Message&) {
        callCount++;
    };

    client_->defineAction(msgType, handler);

    std::byte data[20] = {std::byte{0x01}};

    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(DoAll(SetArrayArgument<0>(data, data + 20), Return(20)));

    EXPECT_CALL(*codec_, tryDecode(_, _))
        .WillOnce([msgType](ByteQueue&, Message& outMsg) {
            outMsg.type() = msgType;
            return DecodeResult{DecodeStatus::Ok, 0, ""};
        })
        .WillOnce([msgType](ByteQueue&, Message& outMsg) {
            outMsg.type() = msgType;
            return DecodeResult{DecodeStatus::Ok, 0, ""};
        })
        .WillOnce(Return(DecodeResult{DecodeStatus::NeedMoreData, 0, ""}));

    client_->update();

    EXPECT_EQ(callCount, 2);
}

TEST_F(ClientTest, DefineActionRegistersHandler)
{
    Message::Type msgType = 300;
    ClientHandler handler = [](OutboundPort&, const Message&) {
    };

    // Should not throw
    client_->defineAction(msgType, handler);
}

TEST_F(ClientTest, SendMultipleMessages)
{
    Message msg1(1);
    Message msg2(2);
    Message msg3(3);

    EXPECT_CALL(*codec_, encode(_, _)).Times(3);

    client_->send(msg1);
    client_->send(msg2);
    client_->send(msg3);
}

TEST_F(ClientTest, HandlerReceivesOutboundPort)
{
    Message::Type msgType = 400;
    bool portReceived = false;

    ClientHandler handler = [&]([[maybe_unused]] OutboundPort& port,
                                const Message&) {
        portReceived = true;
        // Could test port.send() here if needed
    };

    client_->defineAction(msgType, handler);

    std::byte data[10] = {std::byte{0x01}};

    EXPECT_CALL(*transport_, recvBytes(_, _))
        .WillOnce(DoAll(SetArrayArgument<0>(data, data + 10), Return(10)));

    EXPECT_CALL(*codec_, tryDecode(_, _))
        .WillOnce([msgType](ByteQueue&, Message& outMsg) {
            outMsg.type() = msgType;
            return DecodeResult{DecodeStatus::Ok, 0, ""};
        })
        .WillRepeatedly(
            Return(DecodeResult{DecodeStatus::NeedMoreData, 0, ""}));

    client_->update();

    EXPECT_TRUE(portReceived);
}
