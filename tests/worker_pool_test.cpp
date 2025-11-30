// tests/worker_pool_test.cpp
#include "threading/worker_pool.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>
#include <vector>

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

TEST(WorkerPoolConcurrencyTest, ParallelWorkersAreFasterThanSingleWorker)
{
    using clock = std::chrono::steady_clock;
    const int numJobs = 8;

    {
        WorkerPool pool(1);
        std::atomic<int> counter{0};

        auto start = clock::now();
        for (int i = 0; i < numJobs; ++i) {
            pool.addJob([&] {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }

        bool ok = wait_until(
            [&] {
                return counter.load(std::memory_order_relaxed) == numJobs;
            },
            std::chrono::milliseconds(5000));

        ASSERT_TRUE(ok);
        auto end = clock::now();
        auto duration1 =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "[Single worker] duration = " << duration1.count()
                  << " ms\n";

        std::atomic<int> counter2{0};
        auto start2 = clock::now();
        {
            WorkerPool pool2(4);
            for (int i = 0; i < numJobs; ++i) {
                pool2.addJob([&] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    counter2.fetch_add(1, std::memory_order_relaxed);
                });
            }

            bool ok2 = wait_until(
                [&] {
                    return counter2.load(std::memory_order_relaxed) == numJobs;
                },
                std::chrono::milliseconds(5000));

            ASSERT_TRUE(ok2);
        }
        auto end2 = clock::now();
        auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(
            end2 - start2);

        std::cout << "[Four workers] duration = " << duration2.count()
                  << " ms\n";

        EXPECT_LT(duration2.count(), duration1.count() * 0.8)
            << "4-worker pool did not significantly outperform 1-worker pool";
    }
}

TEST(WorkerPoolConcurrencyTest, AcceptsJobsFromMultipleProducers)
{
    const int numWorkers = 4;
    const int numProducers = 8;
    const int jobsPerProducer = 200;
    const int totalJobs = numProducers * jobsPerProducer;

    WorkerPool pool(numWorkers);
    std::atomic<int> counter{0};

    std::vector<std::thread> producers;
    producers.reserve(numProducers);

    for (int p = 0; p < numProducers; ++p) {
        producers.emplace_back([&] {
            for (int i = 0; i < jobsPerProducer; ++i) {
                pool.addJob([&] {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    bool ok = wait_until(
        [&] {
            return counter.load(std::memory_order_relaxed) == totalJobs;
        },
        std::chrono::milliseconds(5000));

    ASSERT_TRUE(ok) << "Not all jobs from multiple producers were executed";
    EXPECT_EQ(totalJobs, counter.load());
}

TEST(WorkerPoolConcurrencyTest, DestructorDoesNotDeadlockWhenIdle)
{
    {
        WorkerPool pool(4);
    }
    SUCCEED();
}

TEST(WorkerPoolConcurrencyTest, DestructorWaitsForJobsToFinish)
{
    const int numWorkers = 4;
    const int numJobs = 20;
    std::atomic<int> counter{0};

    {
        WorkerPool pool(numWorkers);
        for (int i = 0; i < numJobs; ++i) {
            pool.addJob([&] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }
    }

    EXPECT_EQ(numJobs, counter.load())
        << "WorkerPool destructor did not wait for all jobs to finish";
}
