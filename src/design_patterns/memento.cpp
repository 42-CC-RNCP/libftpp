
#include <design_patterns/memento/memento.hpp>

using Snapshot = Memento::Snapshot;

Snapshot Memento::save()
{
    Snapshot s;
    _saveToSnapshot(s);
    return s;
}

void Memento::load(const Snapshot& state)
{
    // make a copy avoid to touch the original snapshot
    Snapshot s = state;
    _loadFromSnapshot(s);
}

DataBuffer& Snapshot::io()
{
    return dbuf_;
}

const DataBuffer& Snapshot::io() const
{
    return dbuf_;
}

Snapshot::Snapshot(const Snapshot& other)
{
    _deepCopy(other);
}

Snapshot& Snapshot::operator=(const Snapshot& other)
{
    if (this == &other) {
        return *this;
    }
    _deepCopy(other);
    return *this;
}

void Snapshot::_deepCopy(const Snapshot& other)
{
    dbuf_.clear();
    dbuf_.setLimits(other.dbuf_.limits());
    const std::byte* p = other.dbuf_.data();
    std::size_t n = other.dbuf_.size();

    if (n) {
        dbuf_.writeBytes(std::span<const std::byte>(p, n));
    }
}
