// tests/observer_test.cpp
#include <design_patterns/observer.hpp>
#include <gtest/gtest.h>
#include <iostream>

enum class EventType
{
    EventA,
    EventB,
    EventC
};

// basic test
TEST(ObserverTest, NotifyCallsSubscribedCallbacks)
{
    Observer<EventType> observer;
    int callCountA = 0;
    int callCountB = 0;

    observer.subscribe(EventType::EventA, [&]() {
        callCountA++;
    });
    observer.subscribe(EventType::EventB, [&]() {
        callCountB++;
    });

    observer.notify(EventType::EventA);
    observer.notify(EventType::EventB);
    observer.notify(EventType::EventA);

    EXPECT_EQ(callCountA, 2);
    EXPECT_EQ(callCountB, 1);
}

TEST(ObserverTest, UnsubscribePreventsFurtherNotifications)
{
    Observer<EventType> observer;
    int callCount = 0;

    observer.subscribe(EventType::EventC, [&]() {
        callCount++;
    });

    observer.notify(EventType::EventC);
    EXPECT_EQ(callCount, 1);

    observer.unsubscribe(EventType::EventC);
    observer.notify(EventType::EventC);
    EXPECT_EQ(callCount, 1); // should not have increased
}

TEST(ObserverTest, NotifyWithoutSubscribersDoesNothing)
{
    Observer<EventType> observer;
    // No subscribers added

    // Just ensure no exceptions are thrown
    EXPECT_NO_THROW(observer.notify(EventType::EventA));
    EXPECT_NO_THROW(observer.notify(EventType::EventB));
    EXPECT_NO_THROW(observer.notify(EventType::EventC));
}

TEST(ObserverTest, MultipleSubscribersForSameEvent)
{
    Observer<EventType> observer;
    int callCount1 = 0;
    int callCount2 = 0;

    observer.subscribe(EventType::EventA, [&]() {
        callCount1++;
    });
    observer.subscribe(EventType::EventA, [&]() {
        callCount2++;
    });

    observer.notify(EventType::EventA);

    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);
}

TEST(ObserverTest, UnsubscribeOneOfMultipleSubscribers)
{
    Observer<EventType> observer;
    int callCount1 = 0;
    int callCount2 = 0;

    observer.subscribe(EventType::EventB, [&]() {
        callCount1++;
    });
    observer.subscribe(EventType::EventB, [&]() {
        callCount2++;
    });

    observer.unsubscribe(EventType::EventB);

    observer.notify(EventType::EventB);

    EXPECT_EQ(callCount1, 0);
    EXPECT_EQ(callCount2, 0);
}

// test with MVC-like scenario

#include <string>
#include <string_view>

enum class WidgetEvent : int
{
    TextChanged
};
struct TextChanged
{
    std::string text;
};

enum class ModelEvent
{
    StateChanged
};
struct AppState
{
    std::string text;
    size_t length;
};

class TextBox
{
public:
    void setText(const std::string& text)
    {
        text_ = text;
        observer.notify(WidgetEvent::TextChanged);
    }

    const std::string& getText() const { return text_; }

    Observer<WidgetEvent> observer;

private:
    std::string text_;
};

class Label
{
public:
    void set_text(std::string_view s)
    {
        text_.assign(s);
        std::cout << "[Label] " << text_ << "\n";
    }

private:
    std::string text_;
};

class Model
{
public:
    void set_text(std::string s)
    {
        state_.text = std::move(s);
        state_.length = state_.text.size();
        observer.notify(ModelEvent::StateChanged);
    }
    const AppState& state() const { return state_; }
    Observer<ModelEvent> observer;

private:
    AppState state_;
};

class Controller
{
public:
    Controller(TextBox& tb, Label& lb, Model& m) : tb_(tb), lb_(lb), m_(m)
    {
        tb_.observer.subscribe(WidgetEvent::TextChanged, [this]() {
            m_.set_text(tb_.getText());
        });

        m_.observer.subscribe(ModelEvent::StateChanged, [this]() {
            const auto& state = m_.state();
            lb_.set_text("Text: " + state.text
                         + ", Length: " + std::to_string(state.length));
        });
    }

private:
    TextBox& tb_;
    Label& lb_;
    Model& m_;
};

TEST(ObserverTest, MvcTextUpdateFlow)
{
    TextBox textbox;
    Label label;
    Model model;
    Controller controller(textbox, label, model);

    textbox.setText("Hello");
    EXPECT_EQ(model.state().text, "Hello");
    EXPECT_EQ(model.state().length, 5u);

    textbox.setText("Observer Pattern");
    EXPECT_EQ(model.state().text, "Observer Pattern");
    EXPECT_EQ(model.state().length, 16u);
}
