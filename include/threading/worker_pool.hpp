// include/threading/worker_pool.hpp
#pragma once
#include "thread.hpp"
#include "thread_safe_queue.hpp"
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

    WorkerPool(size_t numberOfWorkers = 8);
    ~WorkerPool();

    void addJob(const std::function<void()>& jobToExecute);

    // wait for all worker threads to finish
    void joinAllWorkers();

    // stop accepting new jobs and wait for all worker threads to finish
    void stop();

private:
    size_t numberOfWorkers_;
    // 
    ThreadSafeQueue<std::shared_ptr<IJob>> jobQueue_;
    std::vector<std::unique_ptr<Thread>> workers_;
};
