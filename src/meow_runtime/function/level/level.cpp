#include "level.h"

#include "pch.h"

#include "function/components/model/model_component.h"
#include "function/global/runtime_context.h"

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

    std::weak_ptr<GameObject> Level::GetGameObjectByID(UUID go_id) const
    {
        FUNCTION_TIMER();

        auto iter = m_gameobjects.find(go_id);
        if (iter != m_gameobjects.end())
        {
            return iter->second;
        }

        return std::weak_ptr<GameObject>();
    }

    UUID Level::CreateObject()
    {
        FUNCTION_TIMER();

        UUID object_id;

        std::shared_ptr<GameObject> gobject;
        try
        {
            gobject = std::make_shared<GameObject>(object_id);
        }
        catch (const std::bad_alloc&)
        {
            MEOW_ERROR("cannot allocate memory for new gobject");
        }

        m_gameobjects.insert({object_id, gobject});

        return object_id;
    }

    void Level::FrustumCulling()
    {
        m_visibles_per_material.clear();

        std::shared_ptr<GameObject> camera_go_ptr = GetGameObjectByID(m_main_camera_id).lock();

        if (!camera_go_ptr)
            return;

        std::shared_ptr<Camera3DComponent> camera_comp_ptr =
            camera_go_ptr->TryGetComponent<Camera3DComponent>("Camera3DComponent");

        if (!camera_comp_ptr)
        {
            return;
        }

        for (const auto& pair : m_gameobjects)
        {
            // if (camera_comp_ptr->FrustumCulling(pair.second))
            // {
            //     m_visibles[pair.first] = pair.second;
            // }

            std::shared_ptr<ModelComponent> model_comp_ptr =
                pair.second->TryGetComponent<ModelComponent>("ModelComponent");

            if (!model_comp_ptr)
            {
                continue;
            }

            m_visibles_per_material[model_comp_ptr->material_id].push_back(pair.second);
        }
    }
} // namespace Meow
