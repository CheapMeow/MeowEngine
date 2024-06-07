#include "object_id_allocator.h"

#include "core/log/log.h"

namespace Piccolo
{
    std::atomic<GameObjectID> ObjectIDAllocator::m_next_id {0};

    GameObjectID ObjectIDAllocator::alloc()
    {
        std::atomic<GameObjectID> new_object_ret = m_next_id.load();
        m_next_id++;
        if (m_next_id >= k_invalid_gobject_id)
        {
            RUNTIME_ERROR("gobject id overflow");
        }

        return new_object_ret;
    }

} // namespace Piccolo
