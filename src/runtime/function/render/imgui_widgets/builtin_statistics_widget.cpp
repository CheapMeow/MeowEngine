#include "builtin_statistics_widget.h"

#include "imgui_internal.h"
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

                bool per_vertex_bool_arr[17];
                for (int i = 0; i < 17; i++)
                {
                    per_vertex_bool_arr[i] =
                        (bool)(pair.second.per_vertex_attributes & BitMask<VertexAttributeBit>(1 << i));
                }

                if (ImGui::TreeNodeEx("Per Vertex Attributes", flag))
                {
                    for (int i = 0; i < 17; i++)
                    {
                        ImGui::Columns(2, "locations");
                        ImGui::Text("%s", m_attribute_names[i].c_str());
                        ImGui::NextColumn();
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::Checkbox(m_attribute_names[i].c_str(), &per_vertex_bool_arr[i]);
                        ImGui::PopItemFlag();
                        ImGui::Columns();
                    }
                    ImGui::TreePop();
                }

                bool instance_bool_arr[17];
                for (int i = 0; i < 17; i++)
                {
                    instance_bool_arr[i] =
                        (bool)(pair.second.instance_attributes & BitMask<VertexAttributeBit>(1 << i));
                }

                if (ImGui::TreeNodeEx("Instance Attributes", flag))
                {
                    for (int i = 0; i < 17; i++)
                    {
                        ImGui::Columns(2, "locations");
                        ImGui::Text("%s", m_attribute_names[i].c_str());
                        ImGui::NextColumn();
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::Checkbox(m_attribute_names[i].c_str(), &instance_bool_arr[i]);
                        ImGui::PopItemFlag();
                        ImGui::Columns();
                    }
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }
        }
    }
} // namespace Meow