#pragma once

#include "tlv_adapters.hpp"
#include <cstddef> // std::byte, std::size_t
#include <cstring> // std::memcpy
#include <span>
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
    void writeBytes(std::span<const std::byte> s);
    void readExact(std::byte* out, std::size_t n);

    // TODO: phase2: support read from file or socket
    // void append_frame(std::span<std::byte> bytes);

    std::size_t tell() const noexcept { return rd_; }
    void seek(std::size_t pos) {
        if (pos > buf_.size()) throw std::runtime_error("seek past end");
        rd_ = pos;
    }
    const std::byte* data() const;
    std::size_t size() const;
    std::size_t remaining() const noexcept;
    void clear();

    void setLimits(const Limit& limits);
    const Limit& limits() const;

public:
    // move only, not allowed copy
    DataBuffer(const DataBuffer&) = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;

    DataBuffer() = default;
    ~DataBuffer() = default;
    DataBuffer(DataBuffer&&) noexcept = default;
    DataBuffer& operator=(DataBuffer&&) noexcept = default;

private:
    std::vector<std::byte> buf_;
    std::size_t rd_{0};
    Limit limits_{};
};
