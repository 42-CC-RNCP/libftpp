// tests/message_test.cpp
#include "network/message.hpp"
#include <gtest/gtest.h>
#include <string>
#include <type_traits>
#include <vector>

TEST(MessageTest, ConstructorStoresType)
{
    int t = 42;

    Message msg(t);

    EXPECT_EQ(msg.type(), t);
}

TEST(MessageTest, TypeIsNotAffectedByPayloadOperations)
{
    Message msg(1234);
    int x = 10;

    msg << x;

    EXPECT_EQ(msg.type(), 1234);

    int y = 0;
    msg >> y;
    EXPECT_EQ(y, x);
    EXPECT_EQ(msg.type(), 1234);
}

TEST(MessageTest, CanSerializeAndDeserializeSinglePrimitive)
{
    Message msg(1);

    int original = 987654321;
    int decoded = 0;

    msg << original;
    msg >> decoded;

    EXPECT_EQ(decoded, original);
}

TEST(MessageTest, CanSerializeAndDeserializeMultipleFieldsInOrder)
{
    Message msg(99);

    int i_in = 42;
    std::string s_in = "hello, world";
    double d_in = 3.1415926;

    int i_out = 0;
    std::string s_out;
    double d_out = 0.0;

    msg << i_in << s_in << d_in;

    msg >> i_out >> s_out >> d_out;

    EXPECT_EQ(i_out, i_in);
    EXPECT_EQ(s_out, s_in);
    EXPECT_DOUBLE_EQ(d_out, d_in);
}

TEST(MessageTest, SupportsStdVectorSerialization)
{
    Message msg(2);

    std::vector<int> in = {1, 2, 3, 4, 5};
    std::vector<int> out;

    msg << in;
    msg >> out;

    EXPECT_EQ(out.size(), in.size());
    EXPECT_EQ(out, in);
}

TEST(MessageTest, SupportsEnumSerialization)
{
    enum class MyMsgType : int
    {
        Foo = 10,
        Bar = 20,
    };

    Message msg(3);

    MyMsgType in = MyMsgType::Bar;
    MyMsgType out = MyMsgType::Foo;

    msg << in;
    msg >> out;

    EXPECT_EQ(out, in);
}

TEST(MessageTest, OperatorChainingWorks)
{
    Message msg(7);

    int i_in = 111;
    std::string s_in = "chain";
    int i2_in = 222;

    int i_out = 0;
    std::string s_out;
    int i2_out = 0;

    msg << i_in << s_in << i2_in;
    msg >> i_out >> s_out >> i2_out;

    EXPECT_EQ(i_out, i_in);
    EXPECT_EQ(s_out, s_in);
    EXPECT_EQ(i2_out, i2_in);
}

TEST(MessageTest, MismatchedTypeThrowsOnDecode)
{
    Message msg(8);

    int in = 123;
    msg << in;

    std::string out;
    EXPECT_THROW({ msg >> out; }, std::runtime_error);
}

TEST(MessageAdvancedTest, MessageIsMoveOnly)
{
    EXPECT_FALSE(std::is_copy_constructible_v<Message>);
    EXPECT_FALSE(std::is_copy_assignable_v<Message>);
    EXPECT_TRUE(std::is_move_constructible_v<Message>);
    EXPECT_TRUE(std::is_move_assignable_v<Message>);
}

TEST(MessageAdvancedTest, MoveConstructorKeepsTypeAndPayloadIntact)
{
    Message original(42);
    int in = 123456;
    original << in;

    Message moved(std::move(original));

    int out = 0;
    moved >> out;

    EXPECT_EQ(out, in);
    EXPECT_EQ(moved.type(), 42);

    EXPECT_EQ(original.type(), 42);
}

TEST(MessageAdvancedTest, MoveAssignmentKeepsTypeAndPayloadIntact)
{
    Message src(7);
    int in = 999;
    src << in;

    Message dst(1);
    dst = std::move(src);

    int out = 0;
    dst >> out;

    EXPECT_EQ(out, in);
    EXPECT_EQ(dst.type(), 7);
}

TEST(MessageAdvancedTest, ReadingBeyondWrittenPayloadThrows)
{
    Message msg(5);

    int a_in = 111;
    msg << a_in;

    int a_out = 0;
    msg >> a_out;
    EXPECT_EQ(a_out, a_in);

    int extra = 0;
    EXPECT_THROW({ msg >> extra; }, std::runtime_error);
}

TEST(MessageAdvancedTest, AfterDecodeErrorFurtherReadsStillFail)
{
    Message msg(6);

    int in = 42;
    msg << in;

    std::string s;
    EXPECT_THROW({ msg >> s; }, std::runtime_error);

    int out = 0;
    EXPECT_THROW({ msg >> out; }, std::runtime_error);
}

TEST(MessageAdvancedTest, LargeStringWithinDefaultLimitSucceeds)
{
    DataBuffer::Limit limits{};
    const std::size_t msg_limit = limits.max_message_bytes;
    const std::size_t str_limit = limits.max_string_bytes;

    const std::size_t usable_limit = std::min(msg_limit, str_limit);
    const std::size_t len = usable_limit - 32; // leave some room for overhead

    std::string big(len, 'x');
    std::string out;

    Message msg(10);
    msg << big;
    msg >> out;

    EXPECT_EQ(out.size(), big.size());
    EXPECT_EQ(out, big);
}

TEST(MessageAdvancedTest, StringOverDefaultLimitThrows)
{
    const std::size_t limit = 1u << 20;
    std::string tooBig(limit + 1, 'y');

    Message msg(11);

    EXPECT_THROW({ msg << tooBig; }, std::runtime_error);
}

TEST(MessageAdvancedTest, PartialReadDoesNotAffectEarlierFields)
{
    Message msg(12);

    int header = 0xABCD;
    std::string body = "payload";
    int tail = 0x1234;

    msg << header << body << tail;

    int header_out = 0;
    msg >> header_out;
    EXPECT_EQ(header_out, header);
}
