// tests/memento_test.cpp
#include "data_structures/tlv_adapters.hpp"
#include "design_patterns/memento/memento.hpp"
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

TEST(MementoTest, VectorBackend_RoundTrip)
{
    Player p;
    p.name = "Alice";
    p.score = 42;

    auto snap = p.save();

    p.name = "Bob";
    p.score = 7;

    p.load(snap);

    EXPECT_EQ(p.name, "Alice");
    EXPECT_EQ(p.score, 42u);
}

TEST(MementoTest, LoadTwiceFromSameSnapshot_IsIdempotent)
{
    Player p;
    p.name = "Carol";
    p.score = 100;

    auto snap = p.save();

    p.name = "X";
    p.score = 0;
    p.load(snap);
    EXPECT_EQ(p.name, "Carol");
    EXPECT_EQ(p.score, 100u);

    p.name = "Y";
    p.score = 1;
    p.load(snap);
    EXPECT_EQ(p.name, "Carol");
    EXPECT_EQ(p.score, 100u);
}

struct PlayerDB : Player
{
protected:
    SnapIO createBackend() const override
    {
        return SnapIO{DataBufferBackend{}};
    }
};

TEST(MementoTest, DataBufferBackend_RoundTrip)
{
    PlayerDB p;
    p.name = "Dana";
    p.score = 256;

    auto snap = p.save();

    p.name = "Z";
    p.score = 3;
    p.load(snap);

    EXPECT_EQ(p.name, "Dana");
    EXPECT_EQ(p.score, 256u);
}

TEST(MementoTest, CrossBackend_VectorSave_DataBufferLoad)
{
    Player p_vec;
    PlayerDB p_db;

    p_vec.name = "Eva";
    p_vec.score = 777;

    auto snap = p_vec.save();

    p_db.name = "";
    p_db.score = 0;
    p_db.load(snap);

    EXPECT_EQ(p_db.name, "Eva");
    EXPECT_EQ(p_db.score, 777u);
}

TEST(MementoTest, CrossBackend_DataBufferSave_VectorLoad)
{
    PlayerDB p_db; // DataBufferBackend
    Player p_vec;  // VectorBackend

    p_db.name = "Finn";
    p_db.score = 9001;

    auto snap = p_db.save();

    p_vec.name = "??";
    p_vec.score = 0;
    p_vec.load(snap);

    EXPECT_EQ(p_vec.name, "Finn");
    EXPECT_EQ(p_vec.score, 9001u);
}
