#include "data_structures/data_buffer.hpp"
#include <stdexcept> // std::runtime_error

void DataBuffer::writeBytes(std::span<const std::byte> s)
{
    if (s.size() && buf_.size() + s.size() > limits_.max_message_bytes) {
        throw std::runtime_error("message too big");
    }
    buf_.insert(buf_.end(), s.begin(), s.end());
}

void DataBuffer::readExact(std::byte* out, std::size_t n)
{
    if (n > remaining()) {
        throw std::runtime_error("underflow");
    }
    std::memcpy(out, buf_.data() + rd_, n);
    rd_ += n;
}

void DataBuffer::consume(std::size_t n)
{
    if (n > remaining()) {
        throw std::runtime_error("underflow");
    }
    // to avoid O(n) complexity of std::vector::erase
    // buf_.erase(buf_.begin(), buf_.begin() + n);

    // just move the read pointer forward
    rd_ += n;
}

void DataBuffer::compact()
{
    if (rd_ == 0) {
        return; // nothing to do
    }
    if (rd_ >= buf_.size()) {
        // all data consumed, clear the buffer
        buf_.clear();
        rd_ = 0;
        return;
    }
    // to avoid buf OOM issue, use memmove instead of vector erase
    // move remaining data to the front
    std::memmove(buf_.data(), buf_.data() + rd_, buf_.size() - rd_);
    buf_.resize(buf_.size() - rd_);
    rd_ = 0;
}

std::size_t DataBuffer::tell() const noexcept
{
    return rd_;
}

void DataBuffer::seek(std::size_t pos)
{
    if (pos > buf_.size()) {
        throw std::runtime_error("seek past end");
    }
    rd_ = pos;
}

const std::byte* DataBuffer::data() const
{
    return buf_.data() + rd_;
}

std::size_t DataBuffer::size() const
{
    return buf_.size();
}

std::size_t DataBuffer::remaining() const noexcept
{
    return buf_.size() - rd_;
}

void DataBuffer::clear()
{
    buf_.clear();
    rd_ = 0;
}

void DataBuffer::setLimits(const Limit& limits)
{
    limits_ = limits;
}

const DataBuffer::Limit& DataBuffer::limits() const
{
    return limits_;
}
