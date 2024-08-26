#include "pipeline_statistics_widget.h"

#include <imgui.h>

namespace Meow
{
    void PipelineStatisticsWidget::Draw(const std::unordered_map<std::string, std::vector<uint32_t>>& stat)
    {
        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;

        ImGui::PushID(&stat);

        if (ImGui::TreeNodeEx("Pipeline Statistics", flag))
        {
            for (const auto& pair : stat)
            {
                ImGui::PushID(&pair);

                if (ImGui::TreeNodeEx(pair.first.c_str(), flag))
                {
                    if (pair.second.size() < 11)
                        continue;

                    for (int i = 0; i < 11; i++)
                    {
                        ImGui::Columns(2, "locations");
                        ImGui::Text("%s", m_stat_names[i].c_str());
                        ImGui::NextColumn();
                        ImGui::Text("%d", pair.second[i]);
                        ImGui::Columns();
                    }

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }
} // namespace Meow