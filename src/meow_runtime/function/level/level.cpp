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

        std::shared_ptr<GameObject> main_camera = GetGameObjectByID(m_main_camera_id).lock();

        if (!main_camera)
            return;

        std::shared_ptr<Camera3DComponent> main_camera_component =
            main_camera->TryGetComponent<Camera3DComponent>("Camera3DComponent");

        if (!main_camera_component)
        {
            return;
        }

        for (const auto& pair : m_gameobjects)
        {
            // if (main_camera_component->FrustumCulling(pair.second))
            // {
            //     m_visibles[pair.first] = pair.second;
            // }

            std::shared_ptr<ModelComponent> current_gameobject_model_component =
                pair.second->TryGetComponent<ModelComponent>("ModelComponent");

            if (!current_gameobject_model_component)
            {
                continue;
            }

            m_visibles_per_material[current_gameobject_model_component->material_id].push_back(pair.second);
        }
    }
} // namespace Meow
