#pragma once
#include "memento.hpp"
#include <data_structures/tlv_type_traits.hpp>

template <class Derived> class Saveable : public Memento
{
private:
    void _saveToSnapshot(Snapshot& snap) const override
    {
        const auto& self = static_cast<const Derived&>(*this);
        serialize(self, snap.io());
    }
    void _loadFromSnapshot(Snapshot& snap) override
    {
        auto& self = static_cast<Derived&>(*this);
        deserialize(snap.io(), self);
    }
};
