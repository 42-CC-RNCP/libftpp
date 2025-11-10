#pragma once
#include <functional>
#include <unordered_map>
#include <vector>

template <class TEvent> class Observer
{
public:
    void subscribe(const TEvent& event, const std::function<void()>& lambda)
    {
        observers_[event].push_back(lambda);
    }

    void unsubscribe(const TEvent& event) { observers_.erase(event); }
    void notify(const TEvent& event)
    {
        auto it = observers_.find(event);
        if (it == observers_.end()) {
            return;
        }

        auto cbs = it->second; // copy
        for (const auto& cb : cbs) {
            if (cb) {
                cb();
            }
        }
    }

private:
    std::unordered_map<
        TEvent /* hashed event */,
        std::vector<std::function<void()>> /* callback functions*/>
        observers_;
};
