#pragma once

#include "object.hpp"
#include <stack>
#include <vector>

template <typename TType> class Pool
{
public:
    Pool(size_t size);
    ~Pool();

    template <typename... TArgs> TType* acquire(TArgs&&... args);
    void resize(const size_t& numberOfObjectStored);
    void release(TType* obj);

private:
};
