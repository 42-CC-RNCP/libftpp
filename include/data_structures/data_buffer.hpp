#pragma once

#include <cstddef> // std::byte, std::size_t
#include <cstring> // std::memcpy
#include <span>
#include <stdexcept> // std::runtime_error
#include <vector>

class DataBuffer
{
public:
    struct Limit
    {
        std::size_t max_message_bytes = 1 << 20;
        std::size_t max_string_bytes = 1 << 20;
        std::size_t max_depth = 16;
        std::size_t max_elements = 1 << 20;
    };

public:
    void writeBytes(std::span<const std::byte> s)
    {
        if (s.size() && buf_.size() + s.size() > limits_.max_message_bytes) {
            throw std::runtime_error("message too big");
        }
        buf_.insert(buf_.end(), s.begin(), s.end());
    }

    void readExact(std::byte* out, std::size_t n)
    {
        if (n > remaining()) {
            throw std::runtime_error("underflow");
        }
        std::memcpy(out, buf_.data() + rd_, n);
        rd_ += n;
    }

    // TODO: phase2: support read from file or socket
    // void append_frame(std::span<std::byte> bytes);

    const std::byte* data() const { return buf_.data(); }
    std::size_t size() const { return buf_.size(); }
    std::size_t remaining() const noexcept { return buf_.size() - rd_; }
    void clear()
    {
        buf_.clear();
        rd_ = 0;
    }

    void setLimits(const Limit& limits);
    const Limit& limits() const { return limits_; }

private:
    std::vector<std::byte> buf_;
    std::size_t rd_{0};
    Limit limits_{};
};
