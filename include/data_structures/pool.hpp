// include/data_structures/pool.hpp
#pragma once

#include <cstddef>
#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>


template <typename TType>
class Pool
{
public:
    template <typename U> class Object
    {
    public:
        Object(U* p, Pool<U>* pool)
            : _ptr(p), _owner(pool) {}

        Object(Object&& other) noexcept
            : _ptr(other._ptr), _owner(other._owner)
        {
            other._ptr = nullptr;
        }

        Object& operator=(Object&& other) noexcept {
            if (this != &other) {
                _ptr = other._ptr;
                _owner = other._owner;
                other._ptr = nullptr;
            }
            return *this;
        }

        ~Object() {
            if (_ptr && _owner) {
                _owner->release(_ptr);
                _ptr = nullptr;
            }
        }

        U* operator->() { return _ptr; }
        U& operator*()  { return *_ptr; }

        Object(const Object& other) = delete;
        Object& operator=(const Object& other) = delete;

    private:
        U* _ptr;
        Pool<U>* _owner;
    };


    Pool(size_t size) {
        resize(size);
    }
    ~Pool() = default;

    void resize(const size_t& numberOfObjectStored) {
        _storage.clear();
        _available.clear();

        for (size_t i = 0; i < numberOfObjectStored; i++) {
            _storage.emplace_back(std::make_unique<TType>());
            _available.push_back(_storage.back().get());
        }
    }

    template <typename... TArgs>
    Object<TType> acquire(TArgs&&... args) {
        if (_available.empty())
            throw std::runtime_error("no object is available.");

        TType* obj = _available.back();
        _available.pop_back();

        obj->~TType(); // call the destructor but it does not free the memory
        // Re-initialize the object in place:
        //   * std::forward preserves each argument’s original value category
        //     (lvalue or rvalue), enabling perfect forwarding.
        //   * If we wrote  TType(args...)
        //      we would always copy, because every arg is treated as an lvalue.
        //   * If we wrote  TType(std::move(args)...)
        //      we would always move, even when the caller passed an lvalue—often the wrong choice.
        //   * With  TType(std::forward<TArgs>(args)...) the compiler
        //     picks copy or move for each argument as appropriate.

        // use placement new to avoid creating templory instance
        // the TType constructor will be called but not allocate the memory again
        new (obj) TType(std::forward<TArgs>(args)...);
        return Object<TType>(obj, this);
    }

    void release(TType* obj) {
        for (auto& ptr : _storage) {
            if (ptr.get() == obj) {
                _available.push_back(ptr.get());
                return;
            }
        }
    }

private:
    // has the ownership
    // cannot be copy
    // responseble for releasing
    std::vector<std::unique_ptr<TType>> _storage;

    // doesnt have the ownership, only has the pointer, wont be double freed
    std::vector<TType*> _available;
};
