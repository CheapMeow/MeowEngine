#pragma once

#include "core/base/macro.h"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/object/game_object.h"

#include <unordered_map>

namespace Meow
{
    class LIBRARY_API Level
    {
    public:
        void Tick(float dt);

        const std::unordered_map<UUID, std::shared_ptr<GameObject>>& GetAllGameObjects() const { return m_gameobjects; }

        const std::unordered_map<UUID, std::weak_ptr<GameObject>>& GetAllVisibles() const { return m_visibles; }

        std::weak_ptr<GameObject> GetGameObjectByID(UUID go_id) const;

        UUID CreateObject();
        void DeleteGameObjectByID(UUID go_id) { m_gameobjects.erase(go_id); }

        void       SetMainCameraID(UUID go_id) { m_main_camera_id = go_id; }
        const UUID GetMainCameraID() const { return m_main_camera_id; }

    private:
        void FrustumCulling();

        std::unordered_map<UUID, std::shared_ptr<GameObject>> m_gameobjects;
        std::unordered_map<UUID, std::weak_ptr<GameObject>>   m_visibles;

        UUID m_main_camera_id;
    };
} // namespace Meow
