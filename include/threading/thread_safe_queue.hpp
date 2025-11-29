#pragma once
#include <condition_variable>
#include <deque>
#include <mutex>

/*
Exception-safe thread-safe queue implementation using std::deque.
If an exception is thrown during push operations, the queue remains unchanged.
(strong exception safety)
*/
template <typename TType> class ThreadSafeQueue
{
public:
    void push_back(const TType& newElement);
    void push_front(const TType& newElement);
    TType pop_back();
    TType pop_front();
    bool empty();

private:
    std::deque<TType> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};
