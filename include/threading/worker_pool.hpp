// include/threading/worker_pool.hpp
#pragma once
#include "thread.hpp"
#include "thread_safe_queue.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>

/*
main thread (manages job queue)
    |
    |-- worker thread 1
    |-- worker thread 2
    |-- worker thread 3
    |-- ...

worker threads continuously fetch jobs from the job queue and execute them.
*/
class WorkerPool
{
public:
    class IJob
    {
    public:
        virtual ~IJob() = default;
        virtual void execute() = 0;
    };

    WorkerPool(size_t numberOfWorkers = 8) : numberOfWorkers_(numberOfWorkers)
    {
        // default number of worker threads
        // create worker threads
        for (size_t i = 0; i < numberOfWorkers_; ++i) {
            workers_.emplace_back(std::make_unique<Thread>(
                "worker_" + std::to_string(i), [this]() {
                    std::unique_lock<std::mutex> lock(mutex_);
                    // worker thread loop
                    // periodically wake up and run if there are jobs in the
                    // queue and no need to stop
                    while (cv_.wait_for(lock,
                                        std::chrono::milliseconds(100),
                                        [this]() {
                                            return needToStop_.load()
                                                   || !jobQueue_.empty();
                                        }),
                           !needToStop_.load()) {
                        std::shared_ptr<IJob> job;
                        job = jobQueue_.pop_front();
                        job->execute();
                    }
                }));
            workers_.back()->start();
        }
    }

    ~WorkerPool()
    {
        needToStop_.store(true);
        cv_.notify_all();
    }

    void addJob(const std::function<void()>& jobToExecute)
    {
        class JobImpl : public IJob
        {
        public:
            JobImpl(const std::function<void()>& jobFunc) : jobFunc_(jobFunc) {}
            void execute() override { jobFunc_(); }

        private:
            std::function<void()> jobFunc_;
        };

        // the job queue is thread-safe, just push the new job
        jobQueue_.push_back(std::make_shared<JobImpl>(jobToExecute));
    }

private:
    size_t numberOfWorkers_;
    // job queue shared between main thread and worker threads, so use
    // shared_ptr
    ThreadSafeQueue<std::shared_ptr<IJob>> jobQueue_;
    std::vector<std::unique_ptr<Thread>> workers_;
    std::atomic<bool> needToStop_{false};
    std::condition_variable cv_;
    std::mutex mutex_;
};
