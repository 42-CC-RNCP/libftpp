#pragma once
#include <iostream>

// forward declaire
template <typename TType> class Pool;

template <typename TType> class Object
{
public:
    Object(TType* p, Pool<TType>* pool) _ptr(p), _owner(pool);
    Object(Object&& other) noexcept _ptr(other._ptr), _owner(other._owner)
    {
        other._ptr = nullptr;
    };
    Object& operator=(Object&& other) noexcept _ptr(other._ptr),
            _owner(other._owner)
    {
        other._ptr = nullptr;
    };
    ~Object();

    TType* operator->() { return _ptr; }
    TType& operator* { return *_ptr; }

    Object(Object& other) = delete;
    Object& operator=(Object& other) = delete;

private:
    TType* _ptr;
    Pool<TType>* _owner;
}
