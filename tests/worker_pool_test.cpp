// tests/worker_pool_test.cpp
#include "threading/worker_pool.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>

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

TEST(WorkerPoolTest, ExecutesSingleJob)
{
    WorkerPool pool(1);

    std::atomic<int> value{0};
    std::mutex m;
    std::condition_variable cv;
    bool done = false;

    pool.addJob([&] {
        value.store(42, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lk(m);
            done = true;
        }
        cv.notify_one();
    });

    std::unique_lock<std::mutex> lk(m);
    bool finished = cv.wait_for(lk, std::chrono::milliseconds(1000), [&] {
        return done;
    });

    ASSERT_TRUE(finished) << "Worker did not complete job within timeout";
    EXPECT_EQ(42, value.load());
}

TEST(WorkerPoolTest, ExecutesMultipleJobs)
{
    const int numWorkers = 4;
    const int numJobs = 50;

    WorkerPool pool(numWorkers);

    std::atomic<int> counter{0};
    std::mutex m;
    std::condition_variable cv;
    bool allDone = false;

    for (int i = 0; i < numJobs; ++i) {
        pool.addJob([&] {
            int newVal = counter.fetch_add(1, std::memory_order_relaxed) + 1;
            if (newVal == numJobs) {
                std::lock_guard<std::mutex> lk(m);
                allDone = true;
                cv.notify_one();
            }
        });
    }

    std::unique_lock<std::mutex> lk(m);
    bool finished = cv.wait_for(lk, std::chrono::milliseconds(2000), [&] {
        return allDone;
    });

    ASSERT_TRUE(finished) << "Not all jobs completed within timeout";
    EXPECT_EQ(numJobs, counter.load());
}

TEST(WorkerPoolTest, HandlesBurstOfJobs)
{
    const int numWorkers = 8;
    const int numJobs = 1000;

    WorkerPool pool(numWorkers);

    std::atomic<int> counter{0};

    for (int i = 0; i < numJobs; ++i) {
        pool.addJob([&] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    bool ok = wait_until(
        [&] {
            return counter.load() == numJobs;
        },
        std::chrono::milliseconds(5000));

    ASSERT_TRUE(ok) << "Not all burst jobs were executed";
    EXPECT_EQ(numJobs, counter.load());
}
