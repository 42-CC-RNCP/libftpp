// include/threading/thread.hpp
#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <thread>

/**
 * @brief Thread wrapper class for RAII management and per-thread logging.
 *
 * Purpose:
 *   - Provides a simple RAII wrapper around std::thread, ensuring threads are
 * joined on destruction.
 *   - Associates a name with each thread for easier identification and logging.
 *
 * Usage patterns:
 *   - Construct with a thread name and a function to execute.
 *   - Call start() to launch the thread. Throws if called more than once.
 *   - Call stop() to join the thread, or rely on destructor for automatic
 * joining.
 *
 * Thread safety guarantees:
 *   - start() is safe to call once; further calls throw.
 *   - Not copyable or movable to prevent unsafe thread ownership transfer.
 *   - Destructor ensures thread is joined, preventing resource leaks.
 *
 * Relationship with ts_cout:
 *   - Each thread sets a prefix for ts_cout using its name, enabling per-thread
 * logging.
 *   - This helps distinguish log output from different threads.
 */

class Thread
{
public:
    Thread(const std::string& name, std::function<void()> funcToExecute);
    ~Thread();

    // Disable copy and move semantics, as threads should not be copied or
    // moved.
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&&) = delete;
    Thread& operator=(Thread&&) = delete;

    void start();
    void stop();

private:
    std::string name_;
    std::thread thread_;
    std::atomic<bool> started_{false};
    std::function<void()> function_;
};
