#include "threading/persistent_worker.hpp"
#include <vector>

PersistentWorker::PersistentWorker() :
    worker_thread_("persistent_worker", [this]() {
        while (true) {
            std::vector<std::function<void()>> snapshot;
            {
                std::unique_lock<std::mutex> lock(task_mutex_);
                cv_.wait(lock, [this]() {
                    return !tasks_.empty() || stop_.load();
                });
                if (stop_.load()) {
                    break;
                }

                for (const auto& [name, task] : tasks_) {
                    snapshot.push_back(task);
                }
            }

            for (const auto& task : snapshot) {
                task();
            }
        }
    })
{
    worker_thread_.start();
}

PersistentWorker::~PersistentWorker()
{
    stop_.store(true);
    cv_.notify_one();
    worker_thread_.stop();
}

void PersistentWorker::addTask(const std::string& name,
                               const std::function<void()>& jobToExecute)
{
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        tasks_[name] = jobToExecute;
    }
    cv_.notify_one();
}

void PersistentWorker::removeTask(const std::string& name)
{
    std::lock_guard<std::mutex> lock(task_mutex_);
    tasks_.erase(name);
}
