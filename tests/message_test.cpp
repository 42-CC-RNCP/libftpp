// tests/message_test.cpp
#include "network/components/message_builder.hpp"
#include "network/core/message.hpp"
#include <gtest/gtest.h>
#include <string>
#include <type_traits>
#include <vector>

TEST(MessageTest, ConstructorStoresType)
{
    Message::Type t = 42;
    Message msg(t);
    EXPECT_EQ(msg.type(), t);
}

TEST(MessageTest, TypeIsNotAffectedByBytesMutation)
{
    Message msg(1234);
    msg.setBytes({std::byte{0xAA}, std::byte{0xBB}});

    EXPECT_EQ(msg.type(), 1234u);
    EXPECT_EQ(msg.bytes().size(), 2u);
}

TEST(MessageTest, SetBytesReplacesPayload)
{
    Message msg(7);
    std::vector<std::byte> payload{
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};

    msg.setBytes(payload);

    EXPECT_EQ(msg.bytes().size(), payload.size());
    EXPECT_EQ(msg.bytes()[0], std::byte{0x01});
    EXPECT_EQ(msg.bytes()[1], std::byte{0x02});
    EXPECT_EQ(msg.bytes()[2], std::byte{0x03});
}

TEST(MessageTest, BytesReferenceIsMutable)
{
    Message msg(9);
    msg.setBytes({std::byte{0x10}});

    auto& bytes = msg.bytes();
    bytes.push_back(std::byte{0x20});

    EXPECT_EQ(msg.bytes().size(), 2u);
    EXPECT_EQ(msg.bytes()[1], std::byte{0x20});
}

TEST(MessageTest, WriterReaderRoundTripSinglePrimitive)
{
    MessageWriter writer(1);
    int in = 987654321;
    writer << in;

    Message msg = writer.build();

    int out = 0;
    MessageReader reader(msg);
    reader >> out;

    EXPECT_EQ(msg.type(), 1u);
    EXPECT_EQ(out, in);
}

TEST(MessageTest, WriterReaderRoundTripMultipleFieldsInOrder)
{
    MessageWriter writer(99);

    int i_in = 42;
    std::string s_in = "hello, world";
    double d_in = 3.1415926;

    writer << i_in << s_in << d_in;
    Message msg = writer.build();

    int i_out = 0;
    std::string s_out;
    double d_out = 0.0;

    MessageReader reader(msg);
    reader >> i_out >> s_out >> d_out;

    EXPECT_EQ(i_out, i_in);
    EXPECT_EQ(s_out, s_in);
    EXPECT_DOUBLE_EQ(d_out, d_in);
}

TEST(MessageTest, WriterReaderSupportsStdVector)
{
    MessageWriter writer(2);
    std::vector<int> in = {1, 2, 3, 4, 5};
    writer << in;
    Message msg = writer.build();

    std::vector<int> out;
    MessageReader reader(msg);
    reader >> out;

    EXPECT_EQ(out, in);
}

TEST(MessageTest, WriterReaderSupportsEnum)
{
    enum class MyMsgType : int
    {
        Foo = 10,
        Bar = 20,
    };

    MessageWriter writer(3);
    MyMsgType in = MyMsgType::Bar;
    writer << in;
    Message msg = writer.build();

    MyMsgType out = MyMsgType::Foo;
    MessageReader reader(msg);
    reader >> out;

    EXPECT_EQ(out, in);
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
    MessageWriter writer(42);
    int in = 123456;
    writer << in;
    Message original = writer.build();

    Message moved(std::move(original));

    int out = 0;
    MessageReader reader(moved);
    reader >> out;

    EXPECT_EQ(out, in);
    EXPECT_EQ(moved.type(), 42u);
}

TEST(MessageAdvancedTest, MoveAssignmentKeepsTypeAndPayloadIntact)
{
    MessageWriter writer(7);
    int in = 999;
    writer << in;
    Message src = writer.build();

    Message dst(1);
    dst = std::move(src);

    int out = 0;
    MessageReader reader(dst);
    reader >> out;

    EXPECT_EQ(out, in);
    EXPECT_EQ(dst.type(), 7u);
}

TEST(MessageAdvancedTest, ReadingBeyondWrittenPayloadThrows)
{
    MessageWriter writer(5);
    writer << 111;
    Message msg = writer.build();

    MessageReader reader(msg);

    int a_out = 0;
    reader >> a_out;
    EXPECT_EQ(a_out, 111);

    int extra = 0;
    EXPECT_THROW({ reader >> extra; }, std::runtime_error);
}

TEST(MessageAdvancedTest, MismatchedTypeThrowsOnDecode)
{
    MessageWriter writer(8);
    writer << 123;
    Message msg = writer.build();

    MessageReader reader(msg);
    std::string out;
    EXPECT_THROW({ reader >> out; }, std::runtime_error);
}

TEST(MessageAdvancedTest, LargeStringRoundTripSucceeds)
{
    MessageWriter writer(10);
    std::string big(5000, 'x');
    writer << big;
    Message msg = writer.build();

    std::string out;
    MessageReader reader(msg);
    reader >> out;

    EXPECT_EQ(out, big);
}

TEST(MessageAdvancedTest, PartialReadDoesNotAffectEarlierFields)
{
    MessageWriter writer(12);

    int header = 0xABCD;
    std::string body = "payload";
    int tail = 0x1234;

    writer << header << body << tail;
    Message msg = writer.build();

    MessageReader reader(msg);
    int header_out = 0;
    reader >> header_out;

    EXPECT_EQ(header_out, header);
}
