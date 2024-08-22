#pragma once

#include <algorithm>
#include <functional>
#include <vector>

template<typename... Args>
class Signal
{
public:
    using SlotType = std::function<void(Args...)>;

    void connect(const SlotType& slot) { slots.push_back(slot); }

    void disconnect(const SlotType& slot) { slots.erase(std::remove(slots.begin(), slots.end(), slot), slots.end()); }

    void operator()(Args... args) const
    {
        for (const auto& slot : slots)
        {
            slot(args...);
        }
    }

private:
    std::vector<SlotType> slots;
};