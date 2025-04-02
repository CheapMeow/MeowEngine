#pragma once

#include "function/components/camera/camera_3d_component.hpp"
#include "function/object/game_object.h"
#include "function/render/material/shading_model_type.h"

#include <unordered_map>

namespace Meow
{
    class Level
    {
    public:
        void Tick(float dt);

        std::unordered_map<UUID, std::shared_ptr<GameObject>>& GetAllGameObjects() { return m_gameobjects; }

        std::vector<std::weak_ptr<GameObject>>* GetVisiblesPerShadingModel(ShadingModelType shading_model);
        std::weak_ptr<GameObject>               GetGameObjectByID(UUID go_id) const;

        UUID CreateObject();
        void DeleteGameObjectByID(UUID go_id) { m_gameobjects.erase(go_id); }

        void       SetMainCameraID(UUID go_id) { m_main_camera_id = go_id; }
        const UUID GetMainCameraID() const { return m_main_camera_id; }

    private:
        void FrustumCulling();

        std::unordered_map<UUID, std::shared_ptr<GameObject>>                        m_gameobjects;
        std::unordered_map<ShadingModelType, std::vector<std::weak_ptr<GameObject>>> m_visibles_per_shading_model;

        UUID m_main_camera_id;
    };
} // namespace Meow
