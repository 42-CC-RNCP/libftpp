// tests/server_test.cpp
#include "network/components/server.hpp"
#include "network/contracts/message_codec.hpp"
#include "network/contracts/stream_transport.hpp"
#include <cerrno>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <unordered_map>

using ::testing::_;
using ::testing::Return;

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
    MOCK_METHOD(int, nativeHandle, (), (const, override));
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

class OneShotDecodeCodec : public IMessageCodec
{
public:
    explicit OneShotDecodeCodec(Message::Type type) : type_(type) {}

    void encode(const Message&, ByteQueue&) override {}

    DecodeResult tryDecode(ByteQueue&, Message& out) override
    {
        if (!decoded_) {
            decoded_ = true;
            out = Message(type_);
            return {DecodeStatus::Ok, 0, ""};
        }
        return {DecodeStatus::NeedMoreData, 0, ""};
    }

private:
    Message::Type type_;
    bool decoded_{false};
};

class TestReactor : public IReactor
{
public:
    void add(int fd, IoEvent events, IoCallback cb) override
    {
        watchers_[fd] = Watch{events, std::move(cb)};
    }

    void modify(int fd, IoEvent events) override
    {
        auto it = watchers_.find(fd);
        if (it != watchers_.end()) {
            it->second.events = events;
        }
    }

    void remove(int fd) override { watchers_.erase(fd); }

    int poll(int) override
    {
        auto snapshot = watchers_;
        for (auto& [fd, watch] : snapshot) {
            watch.cb(fd, watch.events);
        }
        return static_cast<int>(snapshot.size());
    }

    bool hasFd(int fd) const { return watchers_.count(fd) != 0; }

private:
    struct Watch
    {
        IoEvent events;
        IoCallback cb;
    };

    std::unordered_map<int, Watch> watchers_;
};

TEST(ServerTest, StartCallsListenAndRegistersAcceptorHandle)
{
    auto acceptor = std::make_unique<MockAcceptor>();
    auto* acceptorPtr = acceptor.get();

    auto reactor = std::make_unique<TestReactor>();
    auto* reactorPtr = reactor.get();

    EXPECT_CALL(*acceptorPtr, nativeHandle()).WillRepeatedly(Return(11));
    EXPECT_CALL(*acceptorPtr, listen(8080));
    EXPECT_CALL(*acceptorPtr, tryAccept())
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    Server server(std::move(acceptor), std::move(reactor), []() {
        return std::make_unique<FakeCodec>();
    });

    server.start(8080);

    EXPECT_TRUE(reactorPtr->hasFd(11));
}

TEST(ServerTest, UpdateAcceptsNewConnectionAndRegistersSessionFd)
{
    auto acceptor = std::make_unique<MockAcceptor>();
    auto* acceptorPtr = acceptor.get();

    auto reactor = std::make_unique<TestReactor>();
    auto* reactorPtr = reactor.get();

    auto transport = std::make_unique<MockStreamTransport>();
    auto* transportPtr = transport.get();

    EXPECT_CALL(*acceptorPtr, nativeHandle()).WillRepeatedly(Return(11));
    EXPECT_CALL(*acceptorPtr, listen(_));
    EXPECT_CALL(*acceptorPtr, tryAccept())
        .WillOnce(Return(std::move(transport)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    EXPECT_CALL(*transportPtr, nativeHandle()).WillRepeatedly(Return(21));
    EXPECT_CALL(*transportPtr, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    Server server(std::move(acceptor), std::move(reactor), []() {
        return std::make_unique<FakeCodec>();
    });

    server.start(9000);
    server.update();

    EXPECT_TRUE(reactorPtr->hasFd(21));
}

TEST(ServerTest, SendToQueuesAndFlushesWhenWritable)
{
    auto acceptor = std::make_unique<MockAcceptor>();
    auto* acceptorPtr = acceptor.get();

    auto reactor = std::make_unique<TestReactor>();

    auto transport = std::make_unique<MockStreamTransport>();
    auto* transportPtr = transport.get();

    EXPECT_CALL(*acceptorPtr, nativeHandle()).WillRepeatedly(Return(11));
    EXPECT_CALL(*acceptorPtr, listen(_));
    EXPECT_CALL(*acceptorPtr, tryAccept())
        .WillOnce(Return(std::move(transport)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    EXPECT_CALL(*transportPtr, nativeHandle()).WillRepeatedly(Return(31));
    EXPECT_CALL(*transportPtr, recvBytes(_, _))
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });
    EXPECT_CALL(*transportPtr, sendBytes(_, _)).WillOnce(Return(8));

    Server server(std::move(acceptor), std::move(reactor), []() {
        return std::make_unique<FakeCodec>();
    });

    server.start(9000);
    server.update();

    Message msg(100);
    server.sendTo(msg, 0);
    server.update();
}

TEST(ServerTest, SendToNonExistentClientDoesNotCrash)
{
    auto acceptor = std::make_unique<MockAcceptor>();
    auto* acceptorPtr = acceptor.get();

    auto reactor = std::make_unique<TestReactor>();

    EXPECT_CALL(*acceptorPtr, nativeHandle()).WillRepeatedly(Return(11));
    EXPECT_CALL(*acceptorPtr, listen(_));
    EXPECT_CALL(*acceptorPtr, tryAccept())
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    Server server(std::move(acceptor), std::move(reactor), []() {
        return std::make_unique<FakeCodec>();
    });

    server.start(9000);

    Message msg(500);
    server.sendTo(msg, 999);
    server.update();
}

TEST(ServerTest, HandlerIsCalledForDecodedMessage)
{
    constexpr Message::Type kType = 600;

    auto acceptor = std::make_unique<MockAcceptor>();
    auto* acceptorPtr = acceptor.get();

    auto reactor = std::make_unique<TestReactor>();

    auto transport = std::make_unique<MockStreamTransport>();
    auto* transportPtr = transport.get();

    EXPECT_CALL(*acceptorPtr, nativeHandle()).WillRepeatedly(Return(11));
    EXPECT_CALL(*acceptorPtr, listen(_));
    EXPECT_CALL(*acceptorPtr, tryAccept())
        .WillOnce(Return(std::move(transport)))
        .WillRepeatedly([]() -> std::unique_ptr<IStreamTransport> {
            return {};
        });

    EXPECT_CALL(*transportPtr, nativeHandle()).WillRepeatedly(Return(41));
    EXPECT_CALL(*transportPtr, recvBytes(_, _))
        .WillOnce([](std::byte* buf, size_t) -> ssize_t {
            buf[0] = std::byte{0x01};
            return 1;
        })
        .WillRepeatedly([](std::byte*, size_t) -> ssize_t {
            errno = EAGAIN;
            return -1;
        });

    bool called = false;

    Server server(std::move(acceptor), std::move(reactor), [kType]() {
        return std::make_unique<OneShotDecodeCodec>(kType);
    });

    server.defineAction(kType, [&](PeerHandle&, const Message&) {
        called = true;
    });

    server.start(9000);
    server.update();
    server.update();

    EXPECT_TRUE(called);
}
