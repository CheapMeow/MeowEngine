#include "builtin_statistics_widget.h"

#include "imgui_internal.h"
#include <imgui.h>

#include <map>

namespace Meow
{
    void BuiltinStatisticsWidget::Draw(const std::unordered_map<std::string, BuiltinRenderStat>& stat, size_t& id)
    {
        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::TreeNodeEx(((std::string) "Builtin Statistics" + "##" + std::to_string(id++)).c_str(), flag))
        {
            for (const auto& pair : stat)
            {
                const std::string& pass_name = pair.first;
                const auto&        stat      = pair.second;

                if (ImGui::TreeNodeEx((pass_name + "##" + std::to_string(id++)).c_str(), flag))
                {
                    DrawFrameStat(stat, id);
                    DrawVertexAttributesStat(stat, id);
                    DrawBufferStat(stat, id);
                    DrawImageStat(stat, id);

                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }
    }

    void BuiltinStatisticsWidget::DrawFrameStat(const BuiltinRenderStat& stat, size_t& id)
    {
        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::TreeNodeEx(((std::string) "Frame" + "##" + std::to_string(id++)).c_str(), flag))
        {
            ImGui::Columns(2, "locations");
            ImGui::Text("%s", "Draw call");
            ImGui::NextColumn();
            ImGui::Text("%d", stat.draw_call);
            ImGui::Columns();

            ImGui::TreePop();
        }
    }

    void BuiltinStatisticsWidget::DrawVertexAttributesStat(const BuiltinRenderStat& stat, size_t& id)
    {
        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::TreeNodeEx(((std::string) "Vertex Attributes" + "##" + std::to_string(id++)).c_str(), flag))
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
    }

    void BuiltinStatisticsWidget::DrawBufferStat(const BuiltinRenderStat& stat, size_t& id)
    {
        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::TreeNodeEx(((std::string) "Buffer" + "##" + std::to_string(id++)).c_str(), flag))
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
    }

    void BuiltinStatisticsWidget::DrawImageStat(const BuiltinRenderStat& stat, size_t& id)
    {
        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::TreeNodeEx(((std::string) "Image" + "##" + std::to_string(id++)).c_str(), flag))
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
    }
} // namespace Meow