#pragma once

#include "function/object/game_object.h"

#include <imgui.h>
#include <vector>

namespace Meow
{
    class GameObjectsWidget
    {
    public:
        void Draw(const std::unordered_map<GameObjectID, std::shared_ptr<GameObject>>& gameobject_map);

        const GameObjectID GetSelectedID() const { return m_selected_go_id; }

    private:
        GameObjectID m_selected_go_id;
    };
} // namespace Meow