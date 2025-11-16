#pragma once
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <optional>
#include <unordered_set>

template <class TState> class StateMachine
{
public:
    void addState(const TState& state)
    {
        if (states_.find(state) != states_.end()) {
            throw std::runtime_error("State already exists");
        }
        states_.insert(state);
    }

    void addTransition(const TState& startState,
                       const TState& finalState,
                       const std::function<void()>& lambda)
    {
        if (states_.find(startState) == states_.end()) {
            throw std::runtime_error("Start state does not exist");
        }
        if (states_.find(finalState) == states_.end()) {
            throw std::runtime_error("Final state does not exist");
        }
        auto transIt = transitions_.find(startState);
        if (transIt == transitions_.end()) {
            // No transitions yet for this start state
            transitions_.emplace(
                startState,
                std::unordered_map<TState, std::function<void()>>{
                    {finalState, lambda}});
        }
        else {
            auto& innerMap = transIt->second;
            if (innerMap.find(finalState) != innerMap.end()) {
                throw std::runtime_error("Transition already exists");
            }
            innerMap.emplace(finalState, lambda);
        }
    }

    void addAction(const TState& state, const std::function<void()>& lambda)
    {
        if (states_.find(state) == states_.end()) {
            throw std::runtime_error("State does not exist");
        }
        if (actions_.find(state) != actions_.end()) {
            throw std::runtime_error("Action already exists for this state");
        }
        actions_.emplace(state, lambda);
    }

    void transitionTo(const TState& state)
    {
        if (states_.find(state) == states_.end()) {
            throw std::runtime_error("State does not exist");
        }
        if (currentState_.has_value() == false) {
            // Initial transition, no action
            currentState_ = state;
            return;
        }
        else if (currentState_.value() == state) {
            return; // No transition needed
        }
        auto transIt = transitions_.find(currentState_.value());
        if (transIt == transitions_.end()) {
            throw std::runtime_error(
                "No transitions defined for current state");
        }
        auto innerIt = transIt->second.find(state);
        if (innerIt == transIt->second.end()) {
            throw std::runtime_error("No transition defined to target state");
        }
        // Execute transition action
        innerIt->second();
        currentState_ = state;
    }

    void update()
    {
        if (currentState_.has_value() == false) {
            throw std::runtime_error("No current state set");
        }
        auto actionIt = actions_.find(currentState_.value());
        if (actionIt == actions_.end()) {
            throw std::runtime_error("No action defined for current state");
        }
        actionIt->second();
    }

public:
    StateMachine() = default;
    ~StateMachine() = default;

private:
    std::optional<TState> currentState_; 
    std::unordered_set<TState> states_;
    std::unordered_map<TState, std::function<void()>> actions_;
    // need to use nested map to avoid pair hash issues
    std::unordered_map<
        TState /* start state */,
        std::unordered_map<TState /* end state */, std::function<void()>>>
        transitions_;
};
