#include "pipeline_statistics_widget.h"

#include <imgui.h>

namespace Meow
{
    void PipelineStatisticsWidget::Draw(const std::unordered_map<std::string, std::vector<uint32_t>>& stat, size_t& id)
    {
        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;

        if (ImGui::TreeNodeEx(((std::string) "Pipeline Statistics" + "##" + std::to_string(id++)).c_str(), flag))
        {
            for (const auto& pair : stat)
            {
                if (ImGui::TreeNodeEx((pair.first + "##" + std::to_string(id++)).c_str(), flag))
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
            }

            ImGui::TreePop();
        }
    }
} // namespace Meow