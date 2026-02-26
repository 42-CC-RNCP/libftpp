// tests/dispatcher_test.cpp
#include "network/dispatcher.hpp"
#include "network/message.hpp"
#include <gtest/gtest.h>
#include <string>

struct TestContext
{
    int counter{0};
    std::string lastMessage;
    Message::Type lastType{0};
};

class DispatcherTest : public ::testing::Test
{
protected:
    DispatcherT<TestContext> dispatcher_;
    TestContext context_;
};

TEST_F(DispatcherTest, DefineActionRegistersHandler)
{
    Message::Type msgType = 100;

    dispatcher_.defineRawAction(
        msgType, [](TestContext& ctx, [[maybe_unused]] const Message& msg) {
            ctx.counter = 1;
        });

    // Should not throw
}

TEST_F(DispatcherTest, DispatchCallsCorrectHandler)
{
    Message::Type msgType = 42;
    bool handlerCalled = false;

    dispatcher_.defineRawAction(
        msgType, [&](TestContext& ctx, [[maybe_unused]] const Message& msg) {
            handlerCalled = true;
            ctx.counter = 999;
        });

    Message msg(msgType);
    dispatcher_.dispatch(context_, msg);

    EXPECT_TRUE(handlerCalled);
    EXPECT_EQ(context_.counter, 999);
}

TEST_F(DispatcherTest, DispatchPassesCorrectMessageType)
{
    Message::Type msgType = 123;

    dispatcher_.defineRawAction(msgType,
                                [](TestContext& ctx, const Message& msg) {
                                    ctx.lastType = msg.type();
                                });

    Message msg(msgType);
    dispatcher_.dispatch(context_, msg);

    EXPECT_EQ(context_.lastType, msgType);
}

TEST_F(DispatcherTest, DispatchPassesMessagePayload)
{
    Message::Type msgType = 200;

    dispatcher_.defineRawAction(
        msgType, [](TestContext& ctx, const Message& msg_const) {
            // Need to work around const
            Message& msg = const_cast<Message&>(msg_const);
            int value = 0;
            msg >> value;
            ctx.counter = value;
        });

    Message msg(msgType);
    int payload = 777;
    msg << payload;

    dispatcher_.dispatch(context_, msg);

    EXPECT_EQ(context_.counter, 777);
}

TEST_F(DispatcherTest, MultipleHandlersForDifferentTypes)
{
    int handler1Called = 0;
    int handler2Called = 0;
    int handler3Called = 0;

    dispatcher_.defineRawAction(1, [&](TestContext&, const Message&) {
        handler1Called++;
    });

    dispatcher_.defineRawAction(2, [&](TestContext&, const Message&) {
        handler2Called++;
    });

    dispatcher_.defineRawAction(3, [&](TestContext&, const Message&) {
        handler3Called++;
    });

    Message msg1(1);
    Message msg2(2);
    Message msg3(3);

    dispatcher_.dispatch(context_, msg1);
    dispatcher_.dispatch(context_, msg2);
    dispatcher_.dispatch(context_, msg3);

    EXPECT_EQ(handler1Called, 1);
    EXPECT_EQ(handler2Called, 1);
    EXPECT_EQ(handler3Called, 1);
}

TEST_F(DispatcherTest, DispatchToSameHandlerMultipleTimes)
{
    Message::Type msgType = 50;

    dispatcher_.defineRawAction(msgType, [](TestContext& ctx, const Message&) {
        ctx.counter++;
    });

    Message msg1(msgType);
    Message msg2(msgType);
    Message msg3(msgType);

    dispatcher_.dispatch(context_, msg1);
    dispatcher_.dispatch(context_, msg2);
    dispatcher_.dispatch(context_, msg3);

    EXPECT_EQ(context_.counter, 3);
}

TEST_F(DispatcherTest, DispatchWithUndefinedHandlerThrows)
{
    Message msg(999);

    EXPECT_THROW(dispatcher_.dispatch(context_, msg), std::runtime_error);
}

TEST_F(DispatcherTest, DefineActionWithDuplicateTypeThrows)
{
    Message::Type msgType = 100;

    dispatcher_.defineRawAction(msgType, [](TestContext&, const Message&) {
    });

    EXPECT_THROW(dispatcher_.defineRawAction(msgType,
                                             [](TestContext&, const Message&) {
                                             }),
                 std::runtime_error);
}

TEST_F(DispatcherTest, HandlerCanModifyContext)
{
    Message::Type msgType = 300;

    dispatcher_.defineRawAction(
        msgType, [](TestContext& ctx, const Message& msg_const) {
            Message& msg = const_cast<Message&>(msg_const);
            std::string str;
            msg >> str;
            ctx.lastMessage = str;
            ctx.counter = 42;
        });

    Message msg(msgType);
    std::string payload = "hello";
    msg << payload;

    dispatcher_.dispatch(context_, msg);

    EXPECT_EQ(context_.lastMessage, "hello");
    EXPECT_EQ(context_.counter, 42);
}

TEST_F(DispatcherTest, ContextIsPassedByReference)
{
    Message::Type msgType = 400;

    dispatcher_.defineRawAction(msgType, [](TestContext& ctx, const Message&) {
        ctx.counter = 100;
    });

    TestContext localContext;
    localContext.counter = 0;

    Message msg(msgType);
    dispatcher_.dispatch(localContext, msg);

    // localContext should be modified
    EXPECT_EQ(localContext.counter, 100);
}

TEST_F(DispatcherTest, HandlerCanReadMultipleFieldsFromMessage)
{
    Message::Type msgType = 500;

    dispatcher_.defineRawAction(
        msgType, [](TestContext& ctx, const Message& msg_const) {
            Message& msg = const_cast<Message&>(msg_const);
            int i = 0;
            std::string s;
            double d = 0.0;
            msg >> i >> s >> d;
            ctx.counter = i;
            ctx.lastMessage = s;
        });

    Message msg(msgType);
    msg << 123 << std::string("test") << 3.14;

    dispatcher_.dispatch(context_, msg);

    EXPECT_EQ(context_.counter, 123);
    EXPECT_EQ(context_.lastMessage, "test");
}

TEST_F(DispatcherTest, ZeroIsValidMessageType)
{
    Message::Type msgType = 0;

    dispatcher_.defineRawAction(msgType, [](TestContext& ctx, const Message&) {
        ctx.counter = 555;
    });

    Message msg(msgType);
    dispatcher_.dispatch(context_, msg);

    EXPECT_EQ(context_.counter, 555);
}

TEST_F(DispatcherTest, LargeMessageTypeIsValid)
{
    Message::Type msgType = 0xFFFFFFFF;

    dispatcher_.defineRawAction(msgType, [](TestContext& ctx, const Message&) {
        ctx.counter = 777;
    });

    Message msg(msgType);
    dispatcher_.dispatch(context_, msg);

    EXPECT_EQ(context_.counter, 777);
}

TEST_F(DispatcherTest, HandlerExceptionDoesNotCrashDispatcher)
{
    Message::Type msgType = 600;

    dispatcher_.defineRawAction(msgType, [](TestContext&, const Message&) {
        throw std::runtime_error("Handler error");
    });

    Message msg(msgType);

    EXPECT_THROW(dispatcher_.dispatch(context_, msg), std::runtime_error);
}

TEST_F(DispatcherTest, ManyHandlersCanBeRegistered)
{
    for (int i = 0; i < 100; ++i) {
        dispatcher_.defineRawAction(i, [i](TestContext& ctx, const Message&) {
            ctx.counter = i;
        });
    }

    // Test a few
    Message msg50(50);
    dispatcher_.dispatch(context_, msg50);
    EXPECT_EQ(context_.counter, 50);

    Message msg99(99);
    dispatcher_.dispatch(context_, msg99);
    EXPECT_EQ(context_.counter, 99);
}

TEST_F(DispatcherTest, HandlerReceivesConstMessage)
{
    Message::Type msgType = 700;
    bool receivedConstMessage = false;

    dispatcher_.defineRawAction(msgType, [&](TestContext&, const Message& msg) {
        // If we can call type() on const Message, it's const-correct
        auto t = msg.type();
        receivedConstMessage = (t == msgType);
    });

    Message msg(msgType);
    dispatcher_.dispatch(context_, msg);

    EXPECT_TRUE(receivedConstMessage);
}
