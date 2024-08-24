#include "builtin_statistics_widget.h"

#include <imgui.h>

namespace Meow
{
    void BuiltinStatisticsWidget::Draw(const std::unordered_map<std::string, BuiltinRenderStat>& stat)
    {
        ImGui::Text("Builtin Statistics");

        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        for (const auto& pair : stat)
        {
            if (ImGui::TreeNodeEx(pair.first.c_str(), flag))
            {
                ImGui::Columns(2, "locations");
                ImGui::Text("%s", "Draw call");
                ImGui::NextColumn();
                ImGui::Text("%d", pair.second.draw_call);
                ImGui::Columns();

                ImGui::TreePop();
            }
        }
    }
} // namespace Meow