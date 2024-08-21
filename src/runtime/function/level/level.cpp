#include "level.h"

namespace Meow
{
    void Level::Tick(float dt)
    {
        for (auto& kv : m_gameobjects)
        {
            kv.second->Tick(dt);
        }
    }

    std::weak_ptr<GameObject> Level::GetGameObjectByID(GameObjectID go_id) const
    {
        auto iter = m_gameobjects.find(go_id);
        if (iter != m_gameobjects.end())
        {
            return iter->second;
        }

        return std::weak_ptr<GameObject>();
    }

    GameObjectID Level::CreateObject()
    {
        GameObjectID object_id = ObjectIDAllocator::alloc();
        ASSERT(object_id != k_invalid_gobject_id);

        std::shared_ptr<GameObject> gobject;
        try
        {
            gobject = std::make_shared<GameObject>(object_id);
        }
        catch (const std::bad_alloc&)
        {
            RUNTIME_ERROR("cannot allocate memory for new gobject");
        }

        m_gameobjects.insert({object_id, gobject});
        gobject->self_weak_ptr = gobject;

        return object_id;
    }
} // namespace Meow
