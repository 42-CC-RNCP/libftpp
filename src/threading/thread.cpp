#include "threading/thread.hpp"

Thread::Thread(const std::string& name, std::function<void()> funcToExecute) :
    name_(name), function_(funcToExecute)
{
}

Thread::~Thread()
{
    // For RAII safety, ensure the thread is joined before destruction
    stop();
}

void Thread::start()
{
    // exception should be thrown if start() is called more than once
    if (started_.exchange(true)) {
        throw std::runtime_error("Thread '" + name_
                                 + "' has already been started.");
    }
    thread_ = std::thread([this]() {
        // here is the worker thread
        // if the exception is thrown, it will call std::terminate
        ts_cout.setPrefix("[" + name_ + "] ");

        // the function should be noexcept, otherwise std::terminate is
        // called
        function_();
    });
}

void Thread::stop()
{
    if (thread_.joinable()) {
        thread_.join();
    }
}
