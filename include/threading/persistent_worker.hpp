#pragma once
#include "threading/thread.hpp"
#include <condition_variable>
#include <map>
#include <mutex>

class PersistentWorker
{
public:
    PersistentWorker();

    ~PersistentWorker();

    void addTask(const std::string& name,
                 const std::function<void()>& jobToExecute);

    void removeTask(const std::string& name);

private:
    Thread worker_thread_;
    std::map<std::string, std::function<void()>> tasks_;
    std::condition_variable cv_;
    std::mutex task_mutex_;
    std::atomic<bool> stop_{false};
};
