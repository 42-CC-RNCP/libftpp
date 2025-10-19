// include/design_patterns/memento/memento.hpp
#pragma once
#include "snapio.hpp"
#include <memory>
#include <utility>

class Memento
{
public:
    class Snapshot
    {
    public:
        Snapshot() : io_(std::make_unique<VectorSnapIO>()) {}
        // force user needs to claim specifically, avoid implicit casting
        explicit Snapshot(std::unique_ptr<SnapIO> io) : io_(std::move(io)) {}
        Snapshot(const Snapshot& rhs) : io_(rhs.io_->clone()) {}
        Snapshot& operator=(const Snapshot& rhs)
        {
            if (this != &rhs) {
                io_ = rhs.io_->clone();
            }
            return *this;
        }
        Snapshot(Snapshot&&) noexcept = default;
        Snapshot& operator=(Snapshot&&) noexcept = default;
        ~Snapshot() = default;

    private:
        std::unique_ptr<SnapIO> io_;
        SnapIO& io() { return *io_; }
        const SnapIO& io() const { return *io_; }

        // only allow memento to use internal io
        friend class Memento;
    };

public:
    // will generate a Snapshot of current state by _saveToSnapshot
    Snapshot save() const
    {
        Snapshot s(createBackend());
        _saveToSnapshot(s);
        return s;
    }
    // will base on the Snapshot to restore the state by _loadFromSnapshot
    void load(const Memento::Snapshot& state)
    {
        Snapshot copy = state;
        _loadFromSnapshot(copy);
    }

protected:
    virtual std::unique_ptr<SnapIO> createBackend() const
    {
        return std::make_unique<VectorSnapIO>();
    }

private:
    virtual void _saveToSnapshot(Snapshot& s) const = 0;
    virtual void _loadFromSnapshot(Snapshot& s) = 0;
};
