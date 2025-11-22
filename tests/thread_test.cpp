// thread_test.cpp
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <threading/thread.hpp>


TEST(ThreadTest, DoesNotRunFunctionBeforeStart)
{
    std::atomic<int> counter{0};


    Thread t("worker", [&]() {
        ++counter;
    });

    EXPECT_EQ(counter.load(), 0);
}

TEST(ThreadTest, RunsFunctionOnceAfterStart)
{
    std::atomic<int> counter{0};

    Thread t("worker", [&]() {
        ++counter;
    });

    t.start();
    t.stop();

    EXPECT_EQ(counter.load(), 1);
}

TEST(ThreadTest, StartCalledTwiceThrows)
{
    std::atomic<int> counter{0};

    Thread t("worker", [&]() {
        ++counter;
    });

    t.start();
    t.stop();

    EXPECT_THROW(t.start(), std::runtime_error);
}

TEST(ThreadTest, StopWaitsForThreadToFinish)
{
    std::atomic<bool> finished{false};

    Thread t("worker", [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        finished.store(true, std::memory_order_release);
    });

    t.start();

    t.stop();

    EXPECT_TRUE(finished.load(std::memory_order_acquire));
}

TEST(ThreadTest, ThreadIsProperlyJoinedOnDestruction)
{
    std::atomic<bool> finished{false};

    {
        Thread t("worker", [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            finished.store(true, std::memory_order_release);
        });

        t.start();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_TRUE(finished.load(std::memory_order_acquire));
}
