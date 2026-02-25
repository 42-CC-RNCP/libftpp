// components/message_builder.hpp
#pragma once
#include "network/core/message.hpp"
#include "network/impl/buffer/byte_queue_adapter.hpp"
#include "network/impl/buffer/tlv_adapters.hpp"

class MessageWriter
{
public:
    explicit MessageWriter(Message::Type type) : msg_(type) {}

    template <typename T> MessageWriter& operator<<(const T& v)
    {
        using namespace tlv_adapt;
        buf_ << v;
        return *this;
    }

    Message build()
    {
        // flush buf_ → msg_.bytes_
        auto span = buf_.peek();
        msg_.setBytes({span.begin(), span.end()});
        return std::move(msg_);
    }

private:
    Message msg_;
    DataBufferByteQueue buf_;
};

class MessageReader
{
public:
    explicit MessageReader(const Message& msg)
    {
        auto& b = msg.bytes();
        buf_.append({b.data(), b.size()});
    }

    template <typename T> MessageReader& operator>>(T& v)
    {
        using namespace tlv_adapt;
        buf_ >> v;
        return *this;
    }

private:
    DataBufferByteQueue buf_;
};
