// include/design_patterns/memento/snapio.hpp
#pragma once
#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

// The adapter to be able to support differnt container
struct SnapIO
{
    virtual void write(const void* p, size_t n) = 0;
    virtual void read(void* p, size_t n) = 0;
    virtual size_t tell() const = 0;
    virtual void seek(size_t pos) = 0;
    virtual size_t size() const = 0;
    virtual std::unique_ptr<SnapIO> clone() const = 0;
    virtual ~SnapIO() = default;
};

class VectorSnapIO : public SnapIO
{
public:
    void write(const void* p, size_t n) override
    {
        auto* b = static_cast<const std::byte*>(p);

        if (pos_ + n > buf_.size()) {
            buf_.resize(pos_ + n);
        }
        std::memcpy(buf_.data() + pos_, b, n);
    }

    void read(void* p, size_t n) override
    {
        std::memcpy(p, buf_.data() + pos_, n);
        pos_ += n;
    }

    size_t tell() const override { return pos_; }

    void seek(size_t pos) override { pos_ = pos; }

    size_t size() const override { return buf_.size(); }

    std::unique_ptr<SnapIO> clone() const override
    {
        auto copy = std::make_unique<VectorSnapIO>();

        copy->buf_ = buf_;
        copy->pos_ = pos_;

        return copy;
    }

private:
    std::vector<std::byte> buf_;
    size_t pos_{0};
};
