#include "threading/thread_safe_queue.hpp"

template <typename TType>
void ThreadSafeQueue<TType>::push_back(const TType& newElement)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // TType's copy/move constructor may throw an exception.
        // but the std container guarantees that if an exception is thrown, the
        // container remains unchanged.
        queue_.push_back(newElement);
    }
    // notify one waiting thread that an element has been added
    cv_.notify_one();
}

template <typename TType>
void ThreadSafeQueue<TType>::push_front(const TType& newElement)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_front(newElement);
    }
    // notify one waiting thread that an element has been added
    cv_.notify_one();
}

template <typename TType> TType ThreadSafeQueue<TType>::pop_back()
{
    // when the exception is thrown, RAII will ensure the mutex is unlocked
    // by stack unwinding
    std::unique_lock<std::mutex> lock(mutex_);
    // blocking current thread until queue is not empty and then lock mutex
    // cv_.wait will automatically unlock the mutex while waiting and
    // re-lock it when notified
    // !!condition_variable wait() may throw std::system_error if the mutex
    // is not locked
    cv_.wait(lock, [this]() {
        return !queue_.empty();
    });
    TType element = queue_.back();

    // the copy/move constructor of TType may throw an exception here
    queue_.pop_back();
    return element;
}

template <typename TType> TType ThreadSafeQueue<TType>::pop_front()
{
    std::unique_lock<std::mutex> lock(mutex_);
    // blocking current thread until queue is not empty and then lock mutex
    cv_.wait(lock, [this]() {
        return !queue_.empty();
    });
    TType element = queue_.front();
    queue_.pop_front();
    return element;
}

template <typename TType> bool ThreadSafeQueue<TType>::empty()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}
