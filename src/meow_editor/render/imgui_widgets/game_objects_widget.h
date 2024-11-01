#pragma once

#include "meow_runtime/function/object/game_object.h"

#include <imgui.h>
#include <vector>

namespace Meow
{
    class GameObjectsWidget
    {
    public:
        void Draw(const std::unordered_map<UUID, std::shared_ptr<GameObject>>& gameobject_map);

        const UUID GetSelectedID() const { return m_selected_go_id; }

    private:
        UUID m_selected_go_id;
    };
} // namespace Meow