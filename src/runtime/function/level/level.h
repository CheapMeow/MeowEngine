#pragma once

#include "function/components/camera/camera_3d_component.hpp"
#include "function/object/game_object.h"

#include <unordered_map>

namespace Meow
{
    class Level
    {
    public:
        void Tick(float dt);

        const std::unordered_map<GameObjectID, std::shared_ptr<GameObject>>& GetAllGameObjects() const
        {
            return m_gameobjects;
        }

        const std::unordered_map<GameObjectID, std::weak_ptr<GameObject>>& GetAllVisibles() const { return m_visibles; }

        std::weak_ptr<GameObject> GetGameObjectByID(GameObjectID go_id) const;

        GameObjectID CreateObject();
        void         DeleteGameObjectByID(GameObjectID go_id) { m_gameobjects.erase(go_id); }

    private:
        void FrustumCulling();

        std::unordered_map<GameObjectID, std::shared_ptr<GameObject>> m_gameobjects;
        std::unordered_map<GameObjectID, std::weak_ptr<GameObject>>   m_visibles;
    };
} // namespace Meow
