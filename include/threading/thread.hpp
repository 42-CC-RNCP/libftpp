#pragma once
#include "iostream/thread_safe_iostream.hpp"
#include <functional>
#include <string>
#include <thread>

class Thread
{
public:
    void start()
    {
        thread_ = std::thread([this]() {
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
        tsiostream_.setPrefix("[" + name_ + "] ");
    }

private:
    std::string name_;
    std::thread thread_;
    std::function<void()> function_;
    ThreadSafeIOStream tsiostream_;
};
