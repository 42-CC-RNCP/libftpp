// tests/memento_history_test.cpp
#include <data_structures/tlv_adapters.hpp>
#include <design_patterns/memento/history.hpp>
#include <design_patterns/memento/memento.hpp>
#include <gtest/gtest.h>

class Player : public Memento
{
public:
    std::string name;
    std::uint64_t score = 0;

private:
    void _saveToSnapshot(Snapshot& s) const override
    {
        using namespace tlv_adapt;
        auto& io = stream(s);
        io << name << score;
    }
    void _loadFromSnapshot(Snapshot& s) override
    {
        using namespace tlv_adapt;
        auto& io = stream(s);
        io >> name >> score;
    }
};

TEST(HistoryTest, BasicUndoRedoRoundTrip)
{
    Player p;
    History h;

    // state A
    p.name = "Alice";
    p.score = 1;
    h.push(p.save());

    // state B
    p.name = "Bob";
    p.score = 2;
    h.push(p.save());

    // state C
    p.name = "Carol";
    p.score = 3;
    h.push(p.save());

    // state C, can undo but not redo
    EXPECT_TRUE(h.canUndo());
    EXPECT_FALSE(h.canRedo());

    // Undo -> back to B state
    ASSERT_TRUE(h.undo(p));
    EXPECT_EQ(p.name, "Bob");
    EXPECT_EQ(p.score, 2u);

    // Undo -> back to A state
    ASSERT_TRUE(h.undo(p));
    EXPECT_EQ(p.name, "Alice");
    EXPECT_EQ(p.score, 1u);

    // can't undo anymore (already at oldest)
    EXPECT_FALSE(h.undo(p));
    EXPECT_FALSE(h.canUndo());
    EXPECT_TRUE(h.canRedo());

    // Redo -> back to B
    ASSERT_TRUE(h.redo(p));
    EXPECT_EQ(p.name, "Bob");
    EXPECT_EQ(p.score, 2u);

    // Redo -> back to C
    ASSERT_TRUE(h.redo(p));
    EXPECT_EQ(p.name, "Carol");
    EXPECT_EQ(p.score, 3u);

    // can't redo anymore (already at newest)
    EXPECT_FALSE(h.redo(p));
    EXPECT_FALSE(h.canRedo());
}

TEST(HistoryTest, PushAfterUndoDropsRedoBranch)
{
    Player p;
    History h;

    // state A
    p.name = "A";
    p.score = 10;
    h.push(p.save());

    // state B
    p.name = "B";
    p.score = 20;
    h.push(p.save());

    // state C
    p.name = "C";
    p.score = 30;
    h.push(p.save());

    // Undo -> back to B
    ASSERT_TRUE(h.undo(p));
    EXPECT_EQ(p.name, "B");
    EXPECT_TRUE(h.canRedo());

    // base on B push D
    p.name = "D";
    p.score = 40;
    h.push(p.save());

    // can't redo anymore (C branch is dropped)
    EXPECT_FALSE(h.canRedo());

    // Undo -> back to B
    ASSERT_TRUE(h.undo(p));
    EXPECT_EQ(p.name, "B");
    ASSERT_TRUE(h.redo(p));
    EXPECT_EQ(p.name, "D");
    EXPECT_EQ(p.score, 40u);
}

TEST(HistoryTest, ClearResetsAll)
{
    Player p;
    History h;

    p.name = "X";
    p.score = 1;
    h.push(p.save());

    p.name = "Y";
    p.score = 2;
    h.push(p.save());

    EXPECT_TRUE(h.canUndo());

    h.clear();
    EXPECT_FALSE(h.canUndo());
    EXPECT_FALSE(h.canRedo());

    p.name = "Z";
    p.score = 3;
    h.push(p.save());
    EXPECT_FALSE(h.canUndo());
    EXPECT_FALSE(h.canRedo());
}

TEST(HistoryTest, RepeatedUndoRedoBoundariesAreSafe)
{
    Player p;
    History h;

    p.name = "One";
    p.score = 1;
    h.push(p.save());
    p.name = "Two";
    p.score = 2;
    h.push(p.save());

    EXPECT_FALSE(h.canRedo());
    EXPECT_FALSE(h.redo(p));

    ASSERT_TRUE(h.undo(p));
    EXPECT_EQ(p.name, "One");
    EXPECT_TRUE(h.canRedo());

    EXPECT_FALSE(h.undo(p));
    EXPECT_TRUE(h.canRedo());

    ASSERT_TRUE(h.redo(p));
    EXPECT_EQ(p.name, "Two");
    EXPECT_FALSE(h.canRedo());
}
