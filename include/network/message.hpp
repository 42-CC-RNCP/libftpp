#pragma once
#include "data_structures/data_buffer.hpp"
#include "data_structures/tlv_adapters.hpp"

class Message
{
    using Type = int;

public:
    Message(int type) : type_(type) {}
    int type() const { return type_; }

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
    DataBuffer buf_;
};
