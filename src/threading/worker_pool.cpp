#include "threading/worker_pool.hpp"

WorkerPool::WorkerPool(size_t numberOfWorkers) :
    numberOfWorkers_(numberOfWorkers)
{
    // default number of worker threads
    // create worker threads
    for (size_t i = 0; i < numberOfWorkers_; i++) {
        workers_.emplace_back(
            std::make_unique<Thread>("worker_" + std::to_string(i), [this]() {
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

WorkerPool::~WorkerPool()
{
    stop();
}

void WorkerPool::addJob(const std::function<void()>& jobToExecute)
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

void WorkerPool::joinAllWorkers()
{
    for (auto& worker : workers_) {
        worker->stop();
    }
}

void WorkerPool::stop()
{
    jobQueue_.close();
    joinAllWorkers();
}
