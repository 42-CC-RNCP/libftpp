#pragma once
#include "iostream/thread_safe_iostream.hpp"
#include <atomic>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>

class Thread
{
public:
    void start()
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
            function_();
        });
    }

    void stop()
    {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

public:
    Thread(const std::string& name, std::function<void()> functToExecute)
    {
        name_ = name;
        function_ = functToExecute;
    }

    ~Thread() { stop(); }
    // Disable copy and move semantics, as threads should not be copied or
    // moved.
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(Thread&&) = delete;

private:
    std::string name_;
    std::thread thread_;
    std::atomic<bool> started_{false};
    std::function<void()> function_;
};
