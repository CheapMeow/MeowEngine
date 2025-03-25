#pragma once

#include <algorithm>
#include <functional>
#include <map>

template<typename... Args>
class Signal
{
public:
    using SlotType = std::function<void(Args...)>;
    using SlotID   = std::size_t;

    SlotID connect(const SlotType& slot)
    {
        m_slots[m_sequence] = slot;
        return m_sequence++;
    }

    bool disconnect(SlotID id)
    {
        auto it = m_slots.find(id);
        if (it != m_slots.end())
        {
            m_slots.erase(it);
            return true;
        }
        return false;
    }

    void operator()(Args... args) const
    {
        for (const auto& kv : m_slots)
        {
            kv.second(args...);
        }
    }

    void clear()
    {
        m_slots.clear();
    }

private:
    SlotID                     m_sequence = 0;
    std::map<SlotID, SlotType> m_slots;
};