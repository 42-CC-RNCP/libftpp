#pragma once
#include "byte_queue_adapter.hpp"
#include "data_structures/tlv_adapters.hpp"

class Message
{
public:
    using Type = int;

public:
    Message(int type) : type_(type), buf_() {}

    // property-like movable accessors
    Type& type() noexcept { return type_; }
    const Type& type() const noexcept { return type_; }
    DataBufferByteQueue& payload() noexcept { return buf_; }
    const DataBufferByteQueue& payload() const noexcept { return buf_; }
    void payload(DataBufferByteQueue&& p) noexcept { buf_ = std::move(p); }
    void payload(std::vector<std::byte>&& bytes)
    {
        buf_.reset(std::move(bytes));
    }

    template <typename T> Message& operator<<(const T& v)
    {
        using namespace tlv_adapt;
        buf_ << v;
        return *this;
    }

    template <typename T> Message& operator>>(T& v)
    {
        using namespace tlv_adapt;
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
