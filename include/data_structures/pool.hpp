// include/data_structures/pool.hpp
#pragma once

#include <cstddef>
#include <iterator>
#include <list>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>


template <typename TType>
class Pool
{
    using Slot = std::optional<TType>;
    using It = typename std::list<Slot>::iterator;

public:
    template <typename U> class Object
    {
    public:
        Object(It it, Pool<U>* owner) : it_(it), owner_(owner) {}

        Object(Object&& other) noexcept
            : it_(other.it_), owner_(other.owner_)
        {
            other.owner_ = nullptr;
        }

        Object& operator=(Object&& other) noexcept {
            if (this != &other) {
                it_ = other.it_;
                owner_ = other.owner_;
                other.owner_ = nullptr;
            }
            return *this;
        }

        ~Object() {
            if (owner_) {
                owner_->_release(it_);
                owner_ = nullptr;
            }
        }

        U* operator->() { return &it_->value(); }
        U& operator*()  { return it_->value(); }
        const U* operator->() const { return &it_->value(); }
        const U& operator*()  const { return it_->value();  }

        Object(const Object& other) = delete;
        Object& operator=(const Object& other) = delete;

    private:
        It it_{};
        Pool<U>* owner_{};
    };


    Pool(size_t n) {
        resize(n);
    }
    ~Pool() = default;

    // const size_t& is worse then just size_t by value as a rule
    void resize(const size_t& numberOfObjectStored) {
        auto newSize = numberOfObjectStored;
        auto S = storage_.size();

        if (newSize > S) {
            size_t expandSize = newSize - S;
            while (expandSize--) {
                storage_.emplace_back(std::nullopt);
                available_.push_back(std::prev(storage_.end()));
            }
            return;
        }

        auto shrinkSize = S - newSize;
        if (shrinkSize > available_.size()) {
            throw std::runtime_error("cannot shrink: objects in use");
        }

        while (shrinkSize--) {
            auto it = available_.back();
            available_.pop_back();
            storage_.erase(it);
        }
    }

    template <typename... TArgs>
    Object<TType> acquire(TArgs&&... args) {
        if (available_.empty())
            throw std::runtime_error("no object is available.");

        It it = available_.back();
        available_.pop_back();

        try {
            // *it = TType(std::forward<TArgs>(args)...);
            it->emplace(std::forward<TArgs>(args)...);
        } catch (...) {
            *it = std::nullopt;
            available_.push_back(it);
            throw;
        }

        return Object<TType>(it, this);
    }

private:
    friend class Object<TType>;
    void _release(It it) {
        *it = std::nullopt;
        available_.push_back(it);
    }

private:
    std::list<Slot> storage_;
    std::vector<It> available_;
};
