#include <data_structures/tlv.hpp>
#include <design_patterns/memento/saveable.hpp>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace demo
{

struct Player : public Saveable<Player>
{
    int hp = 0;
    std::string name;
    std::vector<int> inventory;

    bool operator==(const Player& rhs) const
    {
        return hp == rhs.hp && name == rhs.name && inventory == rhs.inventory;
    }
};

// If your Saveable calls tlv::serialize(self, io), keep these names.
// If it calls unqualified serialize/deserialize, ADL will still find them.
template <class IO> inline void serialize(const demo::Player& p, IO& out)
{
    tlv::write_value(out, p.hp);
    tlv::write_value(out, p.name);
    tlv::write_value(out, p.inventory);
}

template <class IO> inline void deserialize(IO& in, demo::Player& p)
{
    tlv::read_value(in, p.hp);
    tlv::read_value(in, p.name);
    tlv::read_value(in, p.inventory);
}

} // namespace demo

// --------- Tests ---------

TEST(Memento_Saveable, RoundTripRestoresState)
{
    demo::Player a;
    a.hp = 100;
    a.name = "Neo";
    a.inventory = {1, 2, 3};

    // Save snapshot
    auto snap = a.save();

    // Mutate
    a.hp = 1;
    a.name = "Smith";
    a.inventory = {7, 8};

    // Load snapshot (restore)
    a.load(snap);

    EXPECT_EQ(a.hp, 100);
    EXPECT_EQ(a.name, "Neo");
    EXPECT_EQ(a.inventory, (std::vector<int>{1, 2, 3}));
}

TEST(Memento_Saveable, MultipleSnapshotsTimeTravel)
{
    demo::Player a;
    a.hp = 50;
    a.name = "Alice";
    a.inventory = {10};

    auto s0 = a.save(); // T0: (50,"Alice",{10})

    // mutate to T1
    a.hp = 75;
    a.name = "Alice+";
    a.inventory = {10, 11};
    auto s1 = a.save(); // T1

    // mutate to T2
    a.hp = 5;
    a.name = "Corrupted";
    a.inventory = {};

    // Travel back to T0
    a.load(s0);
    EXPECT_EQ(a.hp, 50);
    EXPECT_EQ(a.name, "Alice");
    EXPECT_EQ(a.inventory, (std::vector<int>{10}));

    // Travel forward to T1
    a.load(s1);
    EXPECT_EQ(a.hp, 75);
    EXPECT_EQ(a.name, "Alice+");
    EXPECT_EQ(a.inventory, (std::vector<int>{10, 11}));
}

TEST(Memento_Saveable, SnapshotIsUsableThroughBasePointer)
{
    demo::Player a;
    a.hp = 42;
    a.name = "Base";
    a.inventory = {4, 2};

    Memento* base = &a;
    auto snap = a.save();

    // mutate
    a.hp = 0;
    a.name = "Zero";
    a.inventory.clear();

    // load via base
    base->load(snap);

    EXPECT_EQ(a.hp, 42);
    EXPECT_EQ(a.name, "Base");
    EXPECT_EQ(a.inventory, (std::vector<int>{4, 2}));
}

// If your Snapshot is intentionally move-only (common if it owns a noncopyable
// DataBuffer), this asserts the intended semantics at compile-time. If your
// Snapshot *is* copyable, feel free to delete/adjust these lines.
TEST(Memento_Saveable, SnapshotMoveOnlyButLoadable)
{
    demo::Player a;
    a.hp = 7;
    a.name = "Movy";
    a.inventory = {7, 7, 7};

    auto s = a.save();

    // Move construct
    Memento::Snapshot s2 = std::move(s);

    // mutate and then restore using the moved snapshot
    a.hp = -1;
    a.name = "Oops";
    a.inventory = {};
    a.load(s2);

    EXPECT_EQ(a.hp, 7);
    EXPECT_EQ(a.name, "Movy");
    EXPECT_EQ(a.inventory, (std::vector<int>{7, 7, 7}));
}

// Optional: encode a slightly larger payload to stress the IO slightly
// (strings, vectors).
TEST(Memento_Saveable, LargerPayloadRoundTrip)
{
    demo::Player a;
    a.hp = 9001;
    a.name.assign(2048, 'x'); // long string
    a.inventory.resize(1000);
    for (int i = 0; i < 1000; ++i) {
        a.inventory[i] = i;
    }

    auto snap = a.save();

    a.hp = 0;
    a.name.clear();
    a.inventory.clear();

    a.load(snap);

    ASSERT_EQ(a.hp, 9001);
    ASSERT_EQ(a.name.size(), 2048u);
    EXPECT_TRUE(std::all_of(a.name.begin(), a.name.end(), [](char c) {
        return c == 'x';
    }));
    ASSERT_EQ(a.inventory.size(), 1000u);
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(a.inventory[i], i);
    }
}
