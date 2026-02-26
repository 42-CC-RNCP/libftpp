// tests/byte_queue_test.cpp
#include "network/impl/buffer/byte_queue_adapter.hpp"
#include <gtest/gtest.h>
#include <vector>

TEST(ByteQueueTest, InitialStateIsEmpty)
{
    DataBufferByteQueue queue;

    EXPECT_EQ(queue.remaining(), 0u);
    EXPECT_EQ(queue.data(), nullptr);
}

TEST(ByteQueueTest, AppendIncreasesRemaining)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data{
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};

    queue.append(data);

    EXPECT_EQ(queue.remaining(), 3u);
}

TEST(ByteQueueTest, PeekReturnsCorrectData)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data{
        std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}};
    queue.append(data);

    auto view = queue.peek();

    EXPECT_EQ(view.size(), 3u);
    EXPECT_EQ(view[0], std::byte{0xAA});
    EXPECT_EQ(view[1], std::byte{0xBB});
    EXPECT_EQ(view[2], std::byte{0xCC});
}

TEST(ByteQueueTest, PeekDoesNotModifyRemaining)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data{std::byte{0x01}, std::byte{0x02}};
    queue.append(data);

    auto view1 = queue.peek();
    auto view2 = queue.peek();

    EXPECT_EQ(queue.remaining(), 2u);
    EXPECT_EQ(view1.size(), view2.size());
}

TEST(ByteQueueTest, ConsumeReducesRemaining)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data{
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
    queue.append(data);

    queue.consume(2);

    EXPECT_EQ(queue.remaining(), 1u);
}

TEST(ByteQueueTest, ConsumeAllLeavesQueueEmpty)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data{std::byte{0xFF}};
    queue.append(data);

    queue.consume(1);

    EXPECT_EQ(queue.remaining(), 0u);
}

TEST(ByteQueueTest, ConsumeUpdatesDataPointer)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data{
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
    queue.append(data);

    queue.consume(1);
    auto view = queue.peek();

    EXPECT_EQ(view[0], std::byte{0x02});
    EXPECT_EQ(view[1], std::byte{0x03});
}

TEST(ByteQueueTest, MultipleAppendsAccumulate)
{
    DataBufferByteQueue queue;

    std::vector<std::byte> data1{std::byte{0x01}};
    std::vector<std::byte> data2{std::byte{0x02}, std::byte{0x03}};

    queue.append(data1);
    queue.append(data2);

    EXPECT_EQ(queue.remaining(), 3u);
}

TEST(ByteQueueTest, CompactAfterLargeConsume)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data(1000, std::byte{0xFF});
    queue.append(data);

    // Consume more than half to trigger compaction
    queue.consume(600);

    EXPECT_EQ(queue.remaining(), 400u);
    auto view = queue.peek();
    EXPECT_EQ(view[0], std::byte{0xFF});
}

TEST(ByteQueueTest, ExplicitCompactWorks)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data{
        std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}};
    queue.append(data);

    queue.consume(1);
    queue.compact();

    EXPECT_EQ(queue.remaining(), 2u);
    auto view = queue.peek();
    EXPECT_EQ(view[0], std::byte{0xBB});
}

TEST(ByteQueueTest, ResetClearsExistingData)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> initial{std::byte{0x01}, std::byte{0x02}};
    queue.append(initial);

    std::vector<std::byte> newData{std::byte{0xAA}};
    queue.reset(std::move(newData));

    EXPECT_EQ(queue.remaining(), 1u);
    auto view = queue.peek();
    EXPECT_EQ(view[0], std::byte{0xAA});
}

TEST(ByteQueueTest, ResetWithEmptyVector)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data{std::byte{0xFF}};
    queue.append(data);

    std::vector<std::byte> empty;
    queue.reset(std::move(empty));

    EXPECT_EQ(queue.remaining(), 0u);
}

TEST(ByteQueueTest, AppendEmptySpanDoesNothing)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data{std::byte{0x01}};
    queue.append(data);

    std::span<const std::byte> empty;
    queue.append(empty);

    EXPECT_EQ(queue.remaining(), 1u);
}

TEST(ByteQueueTest, ConsumeZeroDoesNothing)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> data{std::byte{0x01}, std::byte{0x02}};
    queue.append(data);

    queue.consume(0);

    EXPECT_EQ(queue.remaining(), 2u);
}

TEST(ByteQueueTest, LargeDataAppendAndConsume)
{
    DataBufferByteQueue queue;
    std::vector<std::byte> large(10000, std::byte{0x55});

    queue.append(large);
    EXPECT_EQ(queue.remaining(), 10000u);

    queue.consume(5000);
    EXPECT_EQ(queue.remaining(), 5000u);

    auto view = queue.peek();
    EXPECT_EQ(view[0], std::byte{0x55});
}

TEST(ByteQueueTest, SequentialOperations)
{
    DataBufferByteQueue queue;

    // Append
    std::vector<std::byte> d1{std::byte{0x01}, std::byte{0x02}};
    queue.append(d1);
    EXPECT_EQ(queue.remaining(), 2u);

    // Consume partial
    queue.consume(1);
    EXPECT_EQ(queue.remaining(), 1u);

    // Append more
    std::vector<std::byte> d2{std::byte{0x03}, std::byte{0x04}};
    queue.append(d2);
    EXPECT_EQ(queue.remaining(), 3u);

    // Peek
    auto view = queue.peek();
    EXPECT_EQ(view[0], std::byte{0x02});
    EXPECT_EQ(view[1], std::byte{0x03});
    EXPECT_EQ(view[2], std::byte{0x04});
}
