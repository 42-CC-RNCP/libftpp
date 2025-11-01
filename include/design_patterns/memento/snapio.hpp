// include/design_patterns/memento/snapio.hpp
#pragma once
#include "data_structures/data_buffer.hpp"
#include <cstddef>
#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

class SnapIO
{
public:
    // template ctor bridge pattern
    // to make any form meet the IOConcept interface wrap to IOModel<Backend>
    template <class Backend>
    SnapIO(Backend&& x) :
        // use decay to remove ref/const qualifer
        pimpl_{std::make_unique<IOModel<std::decay<Backend>>>(
            std::forward<Backend>(x))}
    {
    }
    SnapIO(const SnapIO& rhs) : pimpl_(rhs.pimpl_->clone()) {}
    SnapIO& operator=(const SnapIO& rhs)
    {
        if (this != &rhs) {
            pimpl_ = rhs.pimpl_->clone();
        }
        return *this;
    }
    SnapIO(SnapIO&&) noexcept = default;
    SnapIO& operator=(SnapIO&&) noexcept = default;
    ~SnapIO() = default;

    void write(const void* p, size_t n) { pimpl_->write(p, n); }
    void read(void* p, size_t n) { pimpl_->read(p, n); }
    size_t tell() const { return pimpl_->tell(); }
    void seek(size_t pos) { pimpl_->seek(pos); }
    size_t size() const { return pimpl_->size(); }

private:
    // interface only
    struct IOConcept
    {
        virtual void write(const void* p, size_t n) = 0;
        virtual void read(void* p, size_t n) = 0;
        virtual size_t tell() const = 0;
        virtual void seek(size_t pos) = 0;
        virtual size_t size() const = 0;
        virtual std::unique_ptr<IOConcept> clone() const = 0;
        virtual ~IOConcept() = default;
    };

    template <class Backend> struct IOModel : public IOConcept
    {
        // perfect forwarding to backend constructor
        IOModel(Backend&& x) : b(std::forward(x)) {}
        void write(const void* p, size_t n) override { b.write(p, n); }
        void read(void* p, size_t n) override { b.read(p, n); }
        size_t tell() const override { return b.tell(); }
        void seek(size_t pos) override { b.seek(pos); }
        size_t size() const override { return b.size(); }
        std::unique_ptr<IOConcept> clone() const override
        {
            return std::make_unique<IOModel<Backend>>(*this);
        }

        Backend b;
    };

    std::unique_ptr<IOConcept> pimpl_;
};

struct VectorBackend
{
    void write(const void* p, size_t n)
    {
        auto* b = static_cast<const std::byte*>(p);

        if (pos_ + n > buf_.size()) {
            buf_.resize(pos_ + n);
        }
        std::memcpy(buf_.data() + pos_, b, n);
        pos_ += n;
    }

    void read(void* p, size_t n)
    {
        std::memcpy(p, buf_.data() + pos_, n);
        pos_ += n;
    }

    size_t tell() const { return pos_; }

    void seek(size_t pos) { pos_ = pos; }

    size_t size() const { return buf_.size(); }

private:
    std::vector<std::byte> buf_;
    size_t pos_{0};
};

struct DataBufferBackend
{
public:
    DataBufferBackend() : db_(std::make_unique<DataBuffer>()) {}
    explicit DataBufferBackend(DataBuffer::Limit lim) :
        db_(std::make_unique<DataBuffer>())
    {
        db_->setLimits(lim);
    }
    explicit DataBufferBackend(std::unique_ptr<DataBuffer> owned) :
        db_(std::move(owned))
    {
    }

    DataBufferBackend(const DataBufferBackend& rhs) : db_(clone(*rhs.db_))
    {
    }

    DataBufferBackend& operator=(const DataBufferBackend& rhs)
    {
        if (this != &rhs) {
            db_ = clone(*rhs.db_);
        }
        return *this;
    }

    DataBufferBackend(DataBufferBackend&&) noexcept = default;
    DataBufferBackend& operator=(DataBufferBackend&&) noexcept = default;

    void write(const void* p, size_t n)
    {
        auto* b = static_cast<const std::byte*>(p);
        db_->writeBytes(std::span<const std::byte>(b, n));
    }
    void read(void* p, size_t n)
    {
        auto* b = static_cast<std::byte*>(p);
        db_->readExact(b, n);
    }
    size_t tell() const { return db_->tell(); }
    void seek(size_t pos) { db_->seek(pos); }
    size_t size() const { return db_->size(); }

    const DataBuffer& inner() const { return *db_; }

private:
    static std::unique_ptr<DataBuffer> clone(const DataBuffer& src)
    {
        auto dst = std::make_unique<DataBuffer>();
        dst->setLimits(src.limits());
        if (auto n = src.size(); n) {
            dst->writeBytes(std::span<const std::byte>(src.data(), n));
        }
        dst->seek(src.tell());
        return dst;
    }
    std::unique_ptr<DataBuffer> db_;
};

// // The adapter to be able to support differnt container
// struct SnapIO
// {
//     virtual void write(const void* p, size_t n) = 0;
//     virtual void read(void* p, size_t n) = 0;
//     virtual size_t tell() const = 0;
//     virtual void seek(size_t pos) = 0;
//     virtual size_t size() const = 0;
//     virtual std::unique_ptr<SnapIO> clone() const = 0;
//     virtual ~SnapIO() = default;
// };

// class VectorSnapIO : public SnapIO
// {
// public:
//     void write(const void* p, size_t n) override
//     {
//         auto* b = static_cast<const std::byte*>(p);

//         if (pos_ + n > buf_.size()) {
//             buf_.resize(pos_ + n);
//         }
//         std::memcpy(buf_.data() + pos_, b, n);
//     }

//     void read(void* p, size_t n) override
//     {
//         std::memcpy(p, buf_.data() + pos_, n);
//         pos_ += n;
//     }

//     size_t tell() const override { return pos_; }

//     void seek(size_t pos) override { pos_ = pos; }

//     size_t size() const override { return buf_.size(); }

//     std::unique_ptr<SnapIO> clone() const override
//     {
//         auto copy = std::make_unique<VectorSnapIO>();

//         copy->buf_ = buf_;
//         copy->pos_ = pos_;

//         return copy;
//     }

// private:
//     std::vector<std::byte> buf_;
//     size_t pos_{0};
// };
