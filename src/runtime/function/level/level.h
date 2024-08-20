#pragma once

#include "pch.h"

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

        std::weak_ptr<GameObject> GetGameObjectByID(GameObjectID go_id) const;

        GameObjectID CreateObject();
        void         DeleteGameObjectByID(GameObjectID go_id) { m_gameobjects.erase(go_id); }

    private:
        std::unordered_map<GameObjectID, std::shared_ptr<GameObject>> m_gameobjects;
    };
} // namespace Meow
