#pragma once
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

template <class TType> class Singleton
{
public:
    static std::shared_ptr<TType> instance()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto p = ptr_;

        if (!p) {
            throw std::runtime_error("Singleton not instantiated");
        }
        return p;
    }
    template <typename... TArgs> static void instantiate(TArgs&&... p_args)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ptr_) {
            throw std::runtime_error("Singleton already instantiated");
        }
        ptr_ = std::make_shared<TType>(std::forward<TArgs>(p_args)...);
    }
    static void destroy()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ptr_.reset();
    }

private:
    Singleton() = delete;
    ~Singleton() = delete;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

private:
    inline static std::shared_ptr<TType> ptr_{nullptr};
    inline static std::mutex mutex_{};
};
