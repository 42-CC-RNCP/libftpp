// include/design_patterns/memento/history.hpp
#pragma once
#include "design_patterns/memento/memento.hpp"
#include <vector>

class History
{
public:
    void push(const Memento::Snapshot& state)
    {
        // if we are not at the top, drop all redo states
        if (idx_ + 1 < stack_.size()) {
            stack_.erase(stack_.begin() + idx_ + 1, stack_.end());
        }
        stack_.push_back(state);
        idx_ = stack_.size() - 1;
    }

    bool canUndo() const { return idx_ > 0; }

    bool canRedo() const { return idx_ + 1 < stack_.size(); }

    template <class T /* : Memento */> bool undo(T& obj)
    {
        if (!canUndo()) {
            return false;
        }
        --idx_;
        obj.load(stack_[idx_]);
        return true;
    }

    template <class T /* : Memento */> bool redo(T& obj)
    {
        if (!canRedo()) {
            return false;
        }
        ++idx_;
        obj.load(stack_[idx_]);
        return true;
    }

    void clear()
    {
        stack_.clear();
        idx_ = 0;
    }

private:
    std::vector<Memento::Snapshot> stack_;
    std::size_t idx_{0};
};
