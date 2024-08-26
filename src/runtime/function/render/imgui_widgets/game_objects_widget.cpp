#include "game_objects_widget.h"

#include "imgui_internal.h"

namespace Meow
{
    void GameObjectsWidget::Draw(const std::unordered_map<GameObjectID, std::shared_ptr<GameObject>>& gameobject_map)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGui::PushID(&gameobject_map);

        if (ImGui::TreeNodeEx("GameObject", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (const auto& pair : gameobject_map)
            {
                ImGui::PushID(&pair);

                ImGui::SetNextItemOpen(m_selected_go_id == pair.first ? true : false);
                if (ImGui::TreeNodeEx(pair.second->GetName().c_str(), 0))
                {
                    m_selected_go_id = pair.first;

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }
} // namespace Meow