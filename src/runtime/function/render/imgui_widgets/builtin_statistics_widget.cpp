#include "builtin_statistics_widget.h"

#include "imgui_internal.h"
#include <imgui.h>

#include <map>

namespace Meow
{
    void BuiltinStatisticsWidget::Draw(const std::unordered_map<std::string, BuiltinRenderStat>& stat)
    {
        ImGui::PushID(&stat);

        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::TreeNodeEx("Builtin Statistics", flag))
        {
            for (const auto& pair : stat)
            {
                ImGui::PushID(&pair);

                const std::string& pass_name = pair.first;
                const auto&        stat      = pair.second;

                if (ImGui::TreeNodeEx(pass_name.c_str(), flag))
                {
                    DrawFrameStat(stat);
                    DrawVertexAttributesStat(stat);
                    DrawBufferStat(stat);
                    DrawImageStat(stat);

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void BuiltinStatisticsWidget::DrawFrameStat(const BuiltinRenderStat& stat)
    {
        ImGui::PushID(&stat + 1);

        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::TreeNodeEx("Frame", flag))
        {
            ImGui::Columns(2, "locations");
            ImGui::Text("%s", "Draw call");
            ImGui::NextColumn();
            ImGui::Text("%d", stat.draw_call);
            ImGui::Columns();

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void BuiltinStatisticsWidget::DrawVertexAttributesStat(const BuiltinRenderStat& stat)
    {
        ImGui::PushID(&stat + 2);

        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::TreeNodeEx("Vertex Attributes", flag))
        {
            ImGui::Columns(2, "locations");
            ImGui::Text("%s", "Attribute Name");
            ImGui::NextColumn();
            ImGui::Text("%s", "Location");
            ImGui::Columns();

            ImGui::Separator();

            for (const auto& meta : stat.vertex_attribute_metas)
            {
                ImGui::Columns(2, "locations");
                ImGui::Text("%s", to_string(meta.attribute).c_str());
                ImGui::NextColumn();
                ImGui::Text("%d", meta.location);
                ImGui::Columns();
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void BuiltinStatisticsWidget::DrawBufferStat(const BuiltinRenderStat& stat)
    {
        ImGui::PushID(&stat + 3);

        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::TreeNodeEx("Buffer", flag))
        {
            ImGui::Columns(5, "locations");
            ImGui::Text("%s", "Buffer Name");
            ImGui::NextColumn();
            ImGui::Text("%s", "Set");
            ImGui::NextColumn();
            ImGui::Text("%s", "Binding");
            ImGui::NextColumn();
            ImGui::Text("%s", "Buffer Size");
            ImGui::NextColumn();
            ImGui::Text("%s", "Descriptor Type");
            ImGui::Columns();

            ImGui::Separator();

            std::map<int, std::string> ordered_map;

            int max_binding = 0;
            for (const auto& buffer_pair : stat.buffer_meta_map)
            {
                max_binding = max_binding < buffer_pair.second.binding ? buffer_pair.second.binding : max_binding;
            }

            for (const auto& buffer_pair : stat.buffer_meta_map)
            {
                ordered_map[buffer_pair.second.set * max_binding + buffer_pair.second.binding] = buffer_pair.first;
            }

            for (const auto& ordered_pair : ordered_map)
            {
                auto meta = stat.buffer_meta_map.find(ordered_pair.second);
                if (meta != stat.buffer_meta_map.end())
                {
                    ImGui::Columns(5, "locations");
                    ImGui::Text("%s", ordered_pair.second.c_str());
                    ImGui::NextColumn();
                    ImGui::Text("%d", meta->second.set);
                    ImGui::NextColumn();
                    ImGui::Text("%d", meta->second.binding);
                    ImGui::NextColumn();
                    ImGui::Text("%d", meta->second.bufferSize);
                    ImGui::NextColumn();
                    ImGui::Text("%s", to_string(meta->second.descriptorType).c_str());
                    ImGui::Columns();
                }
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void BuiltinStatisticsWidget::DrawImageStat(const BuiltinRenderStat& stat)
    {
        ImGui::PushID(&stat + 4);

        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::TreeNodeEx("Image", flag))
        {
            ImGui::Columns(4, "locations");
            ImGui::Text("%s", "Image Name");
            ImGui::NextColumn();
            ImGui::Text("%s", "Set");
            ImGui::NextColumn();
            ImGui::Text("%s", "Binding");
            ImGui::NextColumn();
            ImGui::Text("%s", "Descriptor Type");
            ImGui::Columns();

            ImGui::Separator();

            std::map<int, std::string> ordered_map;

            int max_binding = 0;
            for (const auto& buffer_pair : stat.image_meta_map)
            {
                max_binding = max_binding < buffer_pair.second.binding ? buffer_pair.second.binding : max_binding;
            }

            for (const auto& buffer_pair : stat.image_meta_map)
            {
                ordered_map[buffer_pair.second.set * max_binding + buffer_pair.second.binding] = buffer_pair.first;
            }

            for (const auto& ordered_pair : ordered_map)
            {
                auto meta = stat.image_meta_map.find(ordered_pair.second);
                if (meta != stat.image_meta_map.end())
                {
                    ImGui::Columns(4, "locations");
                    ImGui::Text("%s", ordered_pair.second.c_str());
                    ImGui::NextColumn();
                    ImGui::Text("%d", meta->second.set);
                    ImGui::NextColumn();
                    ImGui::Text("%d", meta->second.binding);
                    ImGui::NextColumn();
                    ImGui::Text("%s", to_string(meta->second.descriptorType).c_str());
                    ImGui::Columns();
                }
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
} // namespace Meow