// state_machine_test.cpp
#include <design_patterns/state_machine.hpp>
#include <functional>
#include <gtest/gtest.h>
#include <stdexcept>

enum class Light
{
    Red,
    Green,
    Yellow,
    Blink
};

struct Counters
{
    int red_updates{0};
    int green_updates{0};
    int yellow_updates{0};
    int transition_calls{0};
};

class TrafficLightTest : public ::testing::Test
{
protected:
    StateMachine<Light> sm;
    Counters c;

    void SetUp() override
    {
        sm.addState(Light::Red);
        sm.addState(Light::Green);
        sm.addState(Light::Yellow);
        sm.addState(Light::Blink);

        sm.addAction(Light::Red, [this] {
            c.red_updates++;
        });
        sm.addAction(Light::Green, [this] {
            c.green_updates++;
        });
        sm.addAction(Light::Yellow, [this] {
            c.yellow_updates++;
        });

        sm.addTransition(Light::Red, Light::Green, [this] {
            c.transition_calls++;
        });
        sm.addTransition(Light::Green, Light::Yellow, [this] {
            c.transition_calls++;
        });
        sm.addTransition(Light::Yellow, Light::Red, [this] {
            c.transition_calls++;
        });
    }
};

TEST_F(TrafficLightTest,
       InitialTransitionSetsCurrentButDoesNotRunActionAutomatically)
{
    sm.transitionTo(Light::Red);
    EXPECT_EQ(c.red_updates, 0);
    EXPECT_EQ(c.transition_calls, 0);

    sm.update();
    EXPECT_EQ(c.red_updates, 1);
}

TEST_F(TrafficLightTest, HappyPath_CycleRedGreenYellowBackToRed)
{
    sm.transitionTo(Light::Red);
    sm.update(); // Red action
    EXPECT_EQ(c.red_updates, 1);

    sm.transitionTo(Light::Green); // Red -> Green transition
    EXPECT_EQ(c.transition_calls, 1);
    sm.update(); // Green action
    EXPECT_EQ(c.green_updates, 1);

    sm.transitionTo(Light::Yellow); // Green -> Yellow
    EXPECT_EQ(c.transition_calls, 2);
    sm.update(); // Yellow action
    EXPECT_EQ(c.yellow_updates, 1);

    sm.transitionTo(Light::Red); // Yellow -> Red
    EXPECT_EQ(c.transition_calls, 3);
    sm.update(); // Red action again
    EXPECT_EQ(c.red_updates, 2);
}

TEST_F(TrafficLightTest, MissingAction_ThrowsOnUpdate)
{
    sm.transitionTo(Light::Blink);
    EXPECT_THROW(sm.update(), std::runtime_error);
}

TEST_F(TrafficLightTest, MissingTransition_ThrowsOnTransitionTo)
{
    sm.transitionTo(Light::Red);
    EXPECT_THROW(sm.transitionTo(Light::Yellow), std::runtime_error);
}

TEST_F(TrafficLightTest, UpdateBeforeAnyStateIsSet_Throws)
{
    StateMachine<Light> fresh;
    EXPECT_THROW(fresh.update(), std::runtime_error);
}

TEST(StateMachineContracts, AddTransitionOrActionWithUnknownState_Throws)
{
    StateMachine<Light> sm;
    sm.addState(Light::Red);

    EXPECT_THROW(sm.addTransition(Light::Red,
                                  Light::Green,
                                  [] {
                                  }),
                 std::runtime_error);
    EXPECT_THROW(sm.addTransition(Light::Green,
                                  Light::Red,
                                  [] {
                                  }),
                 std::runtime_error);

    EXPECT_THROW(sm.addAction(Light::Green,
                              [] {
                              }),
                 std::runtime_error);

    EXPECT_THROW(sm.transitionTo(Light::Green), std::runtime_error);
}

TEST(StateMachineContracts, DuplicateDefinitions_Throw)
{
    StateMachine<Light> sm;
    sm.addState(Light::Red);
    sm.addState(Light::Green);

    EXPECT_THROW(sm.addState(Light::Red), std::runtime_error);

    sm.addAction(Light::Red, [] {
    });
    EXPECT_THROW(sm.addAction(Light::Red,
                              [] {
                              }),
                 std::runtime_error);

    sm.addTransition(Light::Red, Light::Green, [] {
    });
    EXPECT_THROW(sm.addTransition(Light::Red,
                                  Light::Green,
                                  [] {
                                  }),
                 std::runtime_error);
}
