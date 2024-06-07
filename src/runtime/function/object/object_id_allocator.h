#pragma once

#include <atomic>
#include <limits>

namespace Meow
{
    using GameObjectID = std::size_t;

    constexpr GameObjectID k_invalid_gobject_id = std::numeric_limits<std::size_t>::max();

    class ObjectIDAllocator
    {
    public:
        static GameObjectID alloc();

    private:
        static std::atomic<GameObjectID> m_next_id;
    };
} // namespace Meow
