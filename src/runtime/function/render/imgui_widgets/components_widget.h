#pragma once

#include "function/object/game_object.h"

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

namespace Meow
{
    class ComponentsWidget
    {
    public:
        ComponentsWidget();

        void CreateGameObjectUI(const std::shared_ptr<GameObject> go, size_t& id);

    private:
        void CreateLeafNodeUI(const reflect::refl_shared_ptr<Component> comp_ptr, size_t& id);
        void DrawVecControl(const std::string& label,
                            glm::vec3&         values,
                            size_t&            id,
                            float              reset_value  = 0.0f,
                            float              column_width = 100.0f);

        std::unordered_map<std::string, std::function<void(std::stack<bool>&, const std::string&, void*, size_t&)>>
                         m_editor_ui_creator;
        std::stack<bool> m_node_states;
    };
} // namespace Meow
