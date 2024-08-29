#include "object_id_allocator.h"

#include "pch.h"

namespace Meow
{
    std::atomic<GameObjectID> ObjectIDAllocator::m_next_id {0};

    GameObjectID ObjectIDAllocator::alloc()
    {
        std::atomic<GameObjectID> new_object_ret = m_next_id.load();
        m_next_id++;
        if (m_next_id >= k_invalid_gobject_id)
        {
            MEOW_ERROR("gobject id overflow");
        }

        return new_object_ret;
    }
} // namespace Meow
