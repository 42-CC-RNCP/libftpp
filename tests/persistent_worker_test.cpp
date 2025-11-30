// tests/persistent_worker_test.cpp
#include "threading/persistent_worker.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

template <class Pred>
bool wait_until(
    Pred pred,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
{
    auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::steady_clock::now() - start > timeout) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

TEST(PersistentWorkerTest, SingleTaskRunsRepeatedly)
{
    PersistentWorker worker;

    std::atomic<int> counter{0};
    worker.addTask("task1", [&] {
        counter.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    });

    bool ok = wait_until(
        [&] {
            return counter.load(std::memory_order_relaxed) >= 5;
        },
        std::chrono::milliseconds(2000));

    ASSERT_TRUE(ok) << "Task was not executed repeatedly";
}

TEST(PersistentWorkerTest, MultipleTasksAreExecuted)
{
    PersistentWorker worker;

    std::atomic<int> c1{0};
    std::atomic<int> c2{0};

    worker.addTask("task1", [&] {
        c1.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    });

    worker.addTask("task2", [&] {
        c2.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    });

    bool ok = wait_until(
        [&] {
            return c1.load(std::memory_order_relaxed) >= 3
                   && c2.load(std::memory_order_relaxed) >= 3;
        },
        std::chrono::milliseconds(2000));

    ASSERT_TRUE(ok) << "Not all tasks were executed repeatedly";
}

TEST(PersistentWorkerTest, RemoveTaskStopsOnlyThatTask)
{
    PersistentWorker worker;

    std::atomic<int> c1{0};
    std::atomic<int> c2{0};

    worker.addTask("A", [&] {
        c1.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    });

    worker.addTask("B", [&] {
        c2.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    });

    bool started = wait_until(
        [&] {
            return c1.load(std::memory_order_relaxed) > 0
                   && c2.load(std::memory_order_relaxed) > 0;
        },
        std::chrono::milliseconds(2000));
    ASSERT_TRUE(started) << "Tasks did not start in time";

    int beforeA = c1.load(std::memory_order_relaxed);
    int beforeB = c2.load(std::memory_order_relaxed);

    worker.removeTask("A");

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    int afterA = c1.load(std::memory_order_relaxed);
    int afterB = c2.load(std::memory_order_relaxed);

    EXPECT_LE(afterA, beforeA + 2)
        << "Task A still increasing significantly after removeTask";
    EXPECT_GT(afterB, beforeB + 2)
        << "Task B did not continue running after removing A";
}

TEST(PersistentWorkerTest, DestructorStopsWorkerThread)
{
    std::atomic<int> counter{0};

    {
        PersistentWorker worker;
        worker.addTask("loop", [&] {
            counter.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        });

        bool ok = wait_until(
            [&] {
                return counter.load(std::memory_order_relaxed) >= 3;
            },
            std::chrono::milliseconds(2000));
        ASSERT_TRUE(ok) << "Task did not start in time";
    }

    int valueAfterDestruction = counter.load(std::memory_order_relaxed);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(valueAfterDestruction, counter.load(std::memory_order_relaxed))
        << "Counter changed after PersistentWorker destruction, "
           "background thread may still be running";
}
