#pragma once

#include "function/object/game_object.h"

#include <imgui.h>
#include <vector>

namespace Meow
{
    class GameObjectsWidget
    {
    public:
        void Draw(const std::unordered_map<UUIDv4::UUID, std::shared_ptr<GameObject>>& gameobject_map);

        const UUIDv4::UUID GetSelectedID() const { return m_selected_go_id; }

    private:
        UUIDv4::UUID m_selected_go_id;
    };
} // namespace Meow