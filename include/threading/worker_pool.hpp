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
                    while (true) {
                        // fetch job from the job queue
                        auto job = jobQueue_.pop_front_optional();
                        if (job.has_value() == false) {
                            // job queue is closed and empty, exit the worker
                            // thread
                            break;
                        }
                        // execute the job
                        job->get()->execute();
                    }
                }));
            workers_.back()->start();
        }
    }

    ~WorkerPool() { stop(); }

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

    // wait for all worker threads to finish
    void joinAllWorkers()
    {
        for (auto& worker : workers_) {
            worker->stop();
        }
    }

    // stop accepting new jobs and wait for all worker threads to finish
    void stop()
    {
        jobQueue_.close();
        joinAllWorkers();
    }

private:
    size_t numberOfWorkers_;
    // job queue shared between main thread and worker threads, so use
    // shared_ptr
    ThreadSafeQueue<std::shared_ptr<IJob>> jobQueue_;
    std::vector<std::unique_ptr<Thread>> workers_;
};
