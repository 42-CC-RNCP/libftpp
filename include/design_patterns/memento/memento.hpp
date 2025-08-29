// include/design_patterns/memento/memento.hpp
#pragma once
#include <data_structures/data_buffer.hpp>

class Memento
{
public:
    class Snapshot
    {
    public:
        // warp the IO operations of data buffer
        DataBuffer& io();
        const DataBuffer& io() const;

        template <class T> Snapshot& operator<<(const T& v)
        {
            dbuf_ << v;
            return *this;
        }

        template <class T> Snapshot& operator>>(T& v)
        {
            dbuf_ >> v;
            return *this;
        }

    public:
        Snapshot() = default;
        // the databuffer is not copyable
        Snapshot(const Snapshot&);
        Snapshot& operator=(const Snapshot& other);
        Snapshot(Snapshot&&) noexcept = default;
        Snapshot& operator=(Snapshot&&) noexcept = default;
        ~Snapshot() = default;

    private:
        void _deepCopy(const Snapshot&);

    private:
        DataBuffer dbuf_{};
    };

public:
    // will generate a Snapshot of current state by _saveToSnapshot
    Snapshot save();
    // will base on the Snapshot to restore the state by _loadFromSnapshot
    void load(const Memento::Snapshot& state);

private:
    virtual void _saveToSnapshot(Snapshot& s) const = 0;
    virtual void _loadFromSnapshot(Snapshot& s) = 0;
};
