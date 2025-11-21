// thread_safe_queue_test.cpp
#include <algorithm>
#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <threading/thread_safe_queue.hpp>
#include <vector>

// Basic single-threaded functionality tests
TEST(ThreadSafeQueueTest, PushBackAndPopFrontSingleThread)
{
    ThreadSafeQueue<int> q;
    q.push_back(1);
    q.push_back(2);
    q.push_back(3);

    EXPECT_EQ(q.pop_front(), 1);
    EXPECT_EQ(q.pop_front(), 2);
    EXPECT_EQ(q.pop_front(), 3);
}

TEST(ThreadSafeQueueTest, PushBackAndPopBackSingleThread)
{
    ThreadSafeQueue<int> q;
    q.push_back(1);
    q.push_back(2);
    q.push_back(3);

    EXPECT_EQ(q.pop_back(), 3);
    EXPECT_EQ(q.pop_back(), 2);
    EXPECT_EQ(q.pop_back(), 1);
}

TEST(ThreadSafeQueueTest, PushFrontAndPopFrontSingleThread)
{
    ThreadSafeQueue<int> q;
    q.push_front(1); // [1]
    q.push_front(2); // [2,1]
    q.push_front(3); // [3,2,1]

    EXPECT_EQ(q.pop_front(), 3);
    EXPECT_EQ(q.pop_front(), 2);
    EXPECT_EQ(q.pop_front(), 1);
}

TEST(ThreadSafeQueueTest, PushFrontAndPopBackSingleThread)
{
    ThreadSafeQueue<int> q;
    q.push_front(1); // [1]
    q.push_front(2); // [2,1]
    q.push_front(3); // [3,2,1]

    EXPECT_EQ(q.pop_back(), 1);
    EXPECT_EQ(q.pop_back(), 2);
    EXPECT_EQ(q.pop_back(), 3);
}

TEST(ThreadSafeQueueTest, MixedPushPopSingleThread)
{
    ThreadSafeQueue<int> q;
    q.push_back(1);  // [1]
    q.push_back(2);  // [1,2]
    q.push_front(0); // [0,1,2]
    q.push_back(3);  // [0,1,2,3]

    EXPECT_EQ(q.pop_front(), 0); // [1,2,3]
    EXPECT_EQ(q.pop_back(), 3);  // [1,2]
    EXPECT_EQ(q.pop_front(), 1); // [2]
    EXPECT_EQ(q.pop_back(), 2);  // []
}

TEST(ThreadSafeQueueTest, PopFrontBlocksUntilElementAvailable)
{
    using namespace std::chrono_literals;

    ThreadSafeQueue<int> q;

    std::atomic<bool> popped{false};
    int result = 0;

    std::thread consumer([&] {
        // should block here until an element is available
        result = q.pop_front();
        popped.store(true, std::memory_order_release);
    });

    // Make sure the consumer thread is likely waiting
    std::this_thread::sleep_for(50ms);

    // This point, consumer should still be blocked
    EXPECT_FALSE(popped.load(std::memory_order_acquire));

    // Now push an element to unblock the consumer
    q.push_back(42);

    consumer.join();

    // Now the consumer should have popped the element
    EXPECT_TRUE(popped.load(std::memory_order_acquire));
    EXPECT_EQ(result, 42);
}

// ----------------------
// multiple producers and multiple consumers test
// ----------------------

TEST(ThreadSafeQueueTest, MultipleProducersMultipleConsumers)
{
    ThreadSafeQueue<int> q;

    constexpr int kNumProducers = 4;
    constexpr int kNumConsumers = 4;
    constexpr int kPerProducer = 1000;
    constexpr int kTotalItems = kNumProducers * kPerProducer;
    constexpr int kSentinel = -1;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::vector<std::vector<int>> consumerResults(kNumConsumers);

    // Producers: each produce kPerProducer items
    for (int p = 0; p < kNumProducers; ++p) {
        producers.emplace_back([p, &q] {
            int base = p * kPerProducer;
            for (int i = 0; i < kPerProducer; ++i) {
                q.push_back(base + i);
            }
        });
    }

    // Consumers: each consume until sentinel is received
    for (int c = 0; c < kNumConsumers; ++c) {
        consumers.emplace_back([c, &q, &consumerResults] {
            while (true) {
                int v = q.pop_front();
                if (v < 0) {
                    break; // sentinel
                }
                consumerResults[c].push_back(v);
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    for (int i = 0; i < kNumConsumers; ++i) {
        q.push_back(kSentinel);
    }

    for (auto& t : consumers) {
        t.join();
    }

    std::vector<int> all;
    all.reserve(kTotalItems);
    for (const auto& vec : consumerResults) {
        all.insert(all.end(), vec.begin(), vec.end());
    }

    EXPECT_EQ(all.size(), static_cast<size_t>(kTotalItems));

    // Check for duplicates and missing items
    std::sort(all.begin(), all.end());
    auto uniqueEnd = std::unique(all.begin(), all.end());
    EXPECT_EQ(uniqueEnd, all.end())
        << "Duplicate elements found in queue results";

    for (int i = 0; i < kTotalItems; ++i) {
        EXPECT_EQ(all[i], i);
    }
}

// ----------------------
// Exception safety smoke test (TType that can throw on copy/move)
// ----------------------

struct ThrowOnCopy
{
    int value;
    static bool throw_on_copy;

    ThrowOnCopy(int v = 0) : value(v) {}

    ThrowOnCopy(const ThrowOnCopy& other) : value(other.value)
    {
        if (throw_on_copy) {
            throw std::runtime_error("copy failed");
        }
    }

    ThrowOnCopy& operator=(const ThrowOnCopy& other)
    {
        if (this != &other) {
            if (throw_on_copy) {
                throw std::runtime_error("copy failed");
            }
            value = other.value;
        }
        return *this;
    }
};

bool ThrowOnCopy::throw_on_copy = false;

TEST(ThreadSafeQueueTest, PushBackExceptionDoesNotBreakFutureOperations)
{
    ThreadSafeQueue<ThrowOnCopy> q;
    ThrowOnCopy v(42);

    // assert that push_back throws and the queue remains usable
    ThrowOnCopy::throw_on_copy = true;
    EXPECT_THROW(q.push_back(v), std::runtime_error);

    // turn off exception throwing, then do normal push/pop to verify queue is still usable
    ThrowOnCopy::throw_on_copy = false;
    q.push_back(ThrowOnCopy(1));
    q.push_back(ThrowOnCopy(2));

    auto r1 = q.pop_front();
    auto r2 = q.pop_front();

    EXPECT_EQ(r1.value, 1);
    EXPECT_EQ(r2.value, 2);
}
