#pragma once
#include "byte_queue_adapter.hpp"

class Message
{
public:
    using Type = int;

public:
    Message(int type) : type_(type), buf_() {}

    int type() const { return type_; }
    DataBufferByteQueue& payload() { return buf_; }
    const DataBufferByteQueue& payload() const { return buf_; }

    template <typename T> Message& operator<<(const T& v)
    {
        buf_ << v;
        return *this;
    }

    template <typename T> Message& operator>>(T& v)
    {
        buf_ >> v;
        return *this;
    }

    // forbid copy constructor and copy assignment
    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;

    // allow move operations constructor and move assignment
    Message(Message&& other) noexcept :
        type_(other.type_), buf_(std::move(other.buf_))
    {
    }

    Message& operator=(Message&& other) noexcept
    {
        if (this != &other) {
            type_ = other.type_;
            buf_ = std::move(other.buf_);
        }
        return *this;
    }

private:
    Type type_;
    DataBufferByteQueue buf_;
};
