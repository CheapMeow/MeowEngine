#include "level.h"

#include "pch.h"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    void Level::Tick(float dt)
    {
        FUNCTION_TIMER();

        for (auto& kv : m_gameobjects)
        {
            kv.second->Tick(dt);
        }

        FrustumCulling();
    }

    std::weak_ptr<GameObject> Level::GetGameObjectByID(GameObjectID go_id) const
    {
        FUNCTION_TIMER();

        auto iter = m_gameobjects.find(go_id);
        if (iter != m_gameobjects.end())
        {
            return iter->second;
        }

        return std::weak_ptr<GameObject>();
    }

    GameObjectID Level::CreateObject()
    {
        FUNCTION_TIMER();

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

    void Level::FrustumCulling()
    {
        m_visibles.clear();

        std::shared_ptr<GameObject> camera_go_ptr =
            GetGameObjectByID(g_runtime_global_context.render_system->m_main_camera_id).lock();

        if (!camera_go_ptr)
            return;

        std::shared_ptr<Camera3DComponent> camera_comp_ptr =
            camera_go_ptr->TryGetComponent<Camera3DComponent>("Camera3DComponent");

        if (camera_comp_ptr)
        {
            for (const auto& pair : m_gameobjects)
            {
                if (camera_comp_ptr->FrustumCulling(pair.second))
                {
                    std::cout << "Find!" << std::endl;
                    m_visibles[pair.first] = pair.second;
                }
            }
        }
    }
} // namespace Meow
