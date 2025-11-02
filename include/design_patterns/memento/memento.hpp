// include/design_patterns/memento/memento.hpp
#pragma once
#include "snapio.hpp"
#include <utility>

class Memento
{
public:
    class Snapshot
    {
    public:
        Snapshot() : io_(VectorBackend{}) {}
        // force user needs to claim specifically, avoid implicit casting
        template <class Backend>
        explicit Snapshot(Backend b) : io_(std::forward<Backend>(b))
        {
        }

        // the SnapIO support copy/move so we can use default here
        Snapshot(const Snapshot&) = default;
        Snapshot& operator=(const Snapshot&) = default;
        Snapshot(Snapshot&&) noexcept = default;
        Snapshot& operator=(Snapshot&&) noexcept = default;
        ~Snapshot() = default;

    private:
        SnapIO io_;
        SnapIO& io() { return io_; }
        const SnapIO& io() const { return io_; }

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
    // provide access to Snapshot's io
    // eventhough Snapshot is private nested, Memento can access its private members because of friendship
    // but the derived classes cannot access Snapshot's private members directly
    // so we provide these static helper functions
    static SnapIO& stream(Snapshot& s) { return s.io(); }
    static const SnapIO& stream(const Snapshot& s) { return s.io(); }

    virtual SnapIO createBackend() const { return VectorBackend{}; }

private:
    virtual void _saveToSnapshot(Snapshot& s) const = 0;
    virtual void _loadFromSnapshot(Snapshot& s) = 0;
};
