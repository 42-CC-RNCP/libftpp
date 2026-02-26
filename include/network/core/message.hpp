// network/core/message.hpp
#pragma once
#include <cstdint>
#include <vector>
#include <cstddef>

class Message
{
public:
    using Type = std::uint32_t;

    explicit Message(Type type) : type_(type) {}

    Type type() const noexcept { return type_; }

    // raw bytes access — for codec layer
    const std::vector<std::byte>& bytes() const noexcept { return bytes_; }
    std::vector<std::byte>& bytes() noexcept { return bytes_; }
    void setBytes(std::vector<std::byte> b) { bytes_ = std::move(b); }

    // move only
    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;
    Message(Message&&) noexcept = default;
    Message& operator=(Message&&) noexcept = default;

private:
    Type type_;
    std::vector<std::byte> bytes_;
};
