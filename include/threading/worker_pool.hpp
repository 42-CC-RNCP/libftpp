// include/threading/worker_pool.hpp
#pragma once
#include "thread.hpp"
#include <functional>

class WorkerPool
{
public:
    class IJob
    {
    public:
        virtual ~IJob() = default;
        virtual void execute() = 0;
    };

    void addJob(const std::function<void()>& jobToExecute);

private:
};
