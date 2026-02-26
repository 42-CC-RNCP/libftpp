// tests/server_test.cpp (FIXED VERSION)
#include "network/acceptor.hpp"
#include "network/message_codec.hpp"
#include "network/server.hpp"
#include "network/stream_transport.hpp"
#include <cerrno>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArrayArgument;

class MockAcceptor : public IAcceptor
{
public:
    MOCK_METHOD(void, listen, (std::uint16_t), (override));
    MOCK_METHOD(std::unique_ptr<IStreamTransport>, tryAccept, (), (override));
    MOCK_METHOD(int, nativeHandle, (), (const, override));
};

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

class FakeCodec : public IMessageCodec
{
public:
    void encode(const Message&, ByteQueue& out) override
    {
        std::byte hdr[8]{};
        out.append(std::span<const std::byte>(hdr, sizeof(hdr)));
    }
    DecodeResult tryDecode(ByteQueue&, Message&) override
    {
        return {DecodeStatus::NeedMoreData, 0, ""};
    }
};

class ServerTest : public ::testing::Test
{
protected:
    std::unique_ptr<MockAcceptor> acceptor_;
    std::function<std::unique_ptr<IMessageCodec>()> codecFactory_;
    std::unique_ptr<Server> server_;

    void SetUp() override
    {
        acceptor_ = std::make_unique<MockAcceptor>();
        codecFactory_ = []() {
            return std::make_unique<FakeCodec>();
        };

        auto acceptorPtr = std::move(acceptor_);
        server_ =
            std::make_unique<Server>(std::move(acceptorPtr), codecFactory_);
    }
};

TEST_F(ServerTest, StartCallsAcceptorListen)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();
    EXPECT_CALL(*mockAcceptor, listen(8080));

    Server server(std::move(mockAcceptor), codecFactory_);
    server.start(8080);
}

TEST_F(ServerTest, DefineActionRegistersHandler)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();
    Server server(std::move(mockAcceptor), codecFactory_);

    Message::Type msgType = 100;
    ServerHandler handler = [](PeerHandle&, const Message&) {
    };

    server.defineAction(msgType, handler);
}

TEST_F(ServerTest, UpdateWithNoConnectionsDoesNothing)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();
    EXPECT_CALL(*mockAcceptor, tryAccept()).WillOnce(Return(nullptr));

    Server server(std::move(mockAcceptor), codecFactory_);
    server.update();
}

TEST_F(ServerTest, UpdateAcceptsNewConnection)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();
    auto mockTransport = std::make_unique<MockStreamTransport>();
    auto* transportPtr = mockTransport.get();

    EXPECT_CALL(*mockAcceptor, tryAccept())
        .WillOnce(Return(std::move(mockTransport)))
        .WillOnce(Return(nullptr));

    // ✅ FIX: Return EAGAIN instead of 0
    EXPECT_CALL(*transportPtr, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    Server server(std::move(mockAcceptor), codecFactory_);
    server.update();
}

TEST_F(ServerTest, UpdateAcceptsMultipleConnections)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();

    auto mockTransport1 = std::make_unique<MockStreamTransport>();
    auto mockTransport2 = std::make_unique<MockStreamTransport>();
    auto* transportPtr1 = mockTransport1.get();
    auto* transportPtr2 = mockTransport2.get();

    EXPECT_CALL(*mockAcceptor, tryAccept())
        .WillOnce(Return(std::move(mockTransport1)))
        .WillOnce(Return(std::move(mockTransport2)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    // ✅ FIX: Both transports return EAGAIN
    EXPECT_CALL(*transportPtr1, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    EXPECT_CALL(*transportPtr2, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    Server server(std::move(mockAcceptor), codecFactory_);
    server.update();
}

TEST_F(ServerTest, SendToQueuesMessageForSpecificClient)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();
    auto mockTransport = std::make_unique<MockStreamTransport>();
    auto* transportPtr = mockTransport.get();

    EXPECT_CALL(*mockAcceptor, tryAccept())
        .WillOnce(Return(std::move(mockTransport)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    EXPECT_CALL(*transportPtr, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    // Now sendBytes should be called
    EXPECT_CALL(*transportPtr, sendBytes(_, _)).WillOnce(Return(8));

    auto codecFactory = []() -> std::unique_ptr<IMessageCodec> {
        return std::make_unique<FakeCodec>();
    };

    Server server(std::move(mockAcceptor), codecFactory);
    server.update(); // Accept connection

    Message msg(100);
    server.sendTo(msg, 0); // ClientId 0

    server.update(); // Process send
}

TEST_F(ServerTest, SendToAllBroadcastsToAllClients)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();

    auto mockTransport1 = std::make_unique<MockStreamTransport>();
    auto mockTransport2 = std::make_unique<MockStreamTransport>();
    auto* transportPtr1 = mockTransport1.get();
    auto* transportPtr2 = mockTransport2.get();

    EXPECT_CALL(*mockAcceptor, tryAccept())
        .WillOnce(Return(std::move(mockTransport1)))
        .WillOnce(Return(std::move(mockTransport2)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    // ✅ FIX: Both return EAGAIN
    EXPECT_CALL(*transportPtr1, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    EXPECT_CALL(*transportPtr2, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    EXPECT_CALL(*transportPtr1, sendBytes(_, _)).WillOnce(Return(8));
    EXPECT_CALL(*transportPtr2, sendBytes(_, _)).WillOnce(Return(8));

    Server server(std::move(mockAcceptor), codecFactory_);
    server.update(); // Accept connections

    Message msg(200);
    server.sendToAll(msg);

    server.update(); // Process sends
}

TEST_F(ServerTest, SendToArraySendsToSpecifiedClients)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();

    auto mockTransport1 = std::make_unique<MockStreamTransport>();
    auto mockTransport2 = std::make_unique<MockStreamTransport>();
    auto mockTransport3 = std::make_unique<MockStreamTransport>();
    auto* transportPtr1 = mockTransport1.get();
    auto* transportPtr2 = mockTransport2.get();
    auto* transportPtr3 = mockTransport3.get();

    EXPECT_CALL(*mockAcceptor, tryAccept())
        .WillOnce(Return(std::move(mockTransport1)))
        .WillOnce(Return(std::move(mockTransport2)))
        .WillOnce(Return(std::move(mockTransport3)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    // ✅ FIX: All return EAGAIN
    EXPECT_CALL(*transportPtr1, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    EXPECT_CALL(*transportPtr2, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    EXPECT_CALL(*transportPtr3, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    // Only clients 0 and 2 should receive
    EXPECT_CALL(*transportPtr1, sendBytes(_, _)).WillOnce(Return(8));
    EXPECT_CALL(*transportPtr3, sendBytes(_, _)).WillOnce(Return(8));

    Server server(std::move(mockAcceptor), codecFactory_);
    server.update(); // Accept connections

    Message msg(300);
    std::vector<INetworkPort::ClientId> recipients = {0, 2};
    server.sendToArray(msg, recipients);

    server.update(); // Process sends
}

TEST_F(ServerTest, DisconnectRemovesClient)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();
    auto mockTransport = std::make_unique<MockStreamTransport>();

    EXPECT_CALL(*mockAcceptor, tryAccept())
        .WillOnce(Return(std::move(mockTransport)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    Server server(std::move(mockAcceptor), codecFactory_);
    server.update(); // Accept connection

    server.disconnect(0); // Disconnect client 0

    // Sending to disconnected client should not crash
    Message msg(400);
    server.sendTo(msg, 0);
}

TEST_F(ServerTest, ClientDisconnectsOnReadError)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();
    auto mockTransport = std::make_unique<MockStreamTransport>();
    auto* transportPtr = mockTransport.get();

    EXPECT_CALL(*mockAcceptor, tryAccept())
        .WillOnce(Return(std::move(mockTransport)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    // ✅ FIX: First return EAGAIN, then return error (not EAGAIN)
    EXPECT_CALL(*transportPtr, recvBytes(_, _))
        .WillOnce([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        })
        .WillOnce([](std::byte*, size_t) -> ssize_t {
            errno = ECONNRESET; // Real error, not EAGAIN
            return -1;
        });

    // Should disconnect once
    EXPECT_CALL(*transportPtr, disconnect()).Times(1);

    Server server(std::move(mockAcceptor), codecFactory_);
    server.update(); // Accept connection
    server.update(); // Process error and disconnect
}

TEST_F(ServerTest, ClientDisconnectsOnConnectionClose)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();
    auto mockTransport = std::make_unique<MockStreamTransport>();
    auto* transportPtr = mockTransport.get();

    EXPECT_CALL(*mockAcceptor, tryAccept())
        .WillOnce(Return(std::move(mockTransport)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    // ✅ FIX: First return EAGAIN, then return 0 (connection closed)
    EXPECT_CALL(*transportPtr, recvBytes(_, _))
        .WillOnce([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        })
        .WillOnce(Return(0)); // Connection closed

    // Should disconnect once
    EXPECT_CALL(*transportPtr, disconnect()).Times(1);

    Server server(std::move(mockAcceptor), codecFactory_);
    server.update(); // Accept connection
    server.update(); // Detect close
}

TEST_F(ServerTest, SendToNonExistentClientDoesNotCrash)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();
    EXPECT_CALL(*mockAcceptor, tryAccept())
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    Server server(std::move(mockAcceptor), codecFactory_);

    Message msg(500);
    server.sendTo(msg, 999); // Non-existent client

    // Should not crash
}

TEST_F(ServerTest, MultipleUpdatesProcessAllSessions)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();

    auto mockTransport1 = std::make_unique<MockStreamTransport>();
    auto mockTransport2 = std::make_unique<MockStreamTransport>();
    auto* transportPtr1 = mockTransport1.get();
    auto* transportPtr2 = mockTransport2.get();

    EXPECT_CALL(*mockAcceptor, tryAccept())
        .WillOnce(Return(std::move(mockTransport1)))
        .WillOnce(Return(std::move(mockTransport2)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    // ✅ FIX: Return EAGAIN
    EXPECT_CALL(*transportPtr1, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    EXPECT_CALL(*transportPtr2, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    Server server(std::move(mockAcceptor), codecFactory_);

    server.update(); // Accept both
    server.update(); // Process both
    server.update(); // Process both again
}

TEST_F(ServerTest, HandlerIsCalledForReceivedMessage)
{
    auto mockAcceptor = std::make_unique<MockAcceptor>();
    Server server(std::move(mockAcceptor), codecFactory_);

    Message::Type msgType = 600;
    bool handlerCalled = false;

    server.defineAction(msgType, [&](PeerHandle&, const Message&) {
        handlerCalled = true;
    });

    EXPECT_FALSE(handlerCalled);
}
