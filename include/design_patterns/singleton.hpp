#pragma once
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

template <class TType> class Singleton
{
public:
    static TType* instance()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        TType* p = ptr_.get();

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
        ptr_ = std::make_unique<TType>(std::forward<TArgs>(p_args)...);
    }
    static void destroy()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ptr_.reset(nullptr);
    }

private:
    Singleton() = delete;
    ~Singleton() = delete;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

private:
    inline static std::unique_ptr<TType> ptr_{nullptr};
    inline static std::mutex mutex_{};
};
