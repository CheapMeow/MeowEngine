#include "builtin_statistics_widget.h"

#include "imgui_internal.h"
#include <glm/gtx/color_space.hpp>
#include <map>

namespace Meow
{
    BuiltinStatisticsWidget::BuiltinStatisticsWidget()
    {
        m_col_base_table.reserve(k_gredint_count);
        m_col_hovered_table.reserve(k_gredint_count);

        for (int i = 0; i < k_gredint_count; i++)
        {
            float        saturation_ratio = static_cast<float>(i + 1) / k_gredint_count;
            glm::vec3    red_rgb          = glm::rgbColor(glm::vec3(120.0, saturation_ratio, 0.5));
            unsigned int alpha            = 255;
            unsigned int red              = 255 * red_rgb.x;
            unsigned int green            = 255 * red_rgb.y;
            unsigned int blue             = 255 * red_rgb.z;
            m_col_base_table[i]           = (alpha << 24) | (red << 16) | (green << 8) | blue;
        }
        for (int i = 0; i < k_gredint_count; i++)
        {
            float        saturation_ratio = static_cast<float>(i + 1) / k_gredint_count;
            glm::vec3    red_rgb          = glm::rgbColor(glm::vec3(120.0, saturation_ratio, 0.8));
            unsigned int alpha            = 255;
            unsigned int red              = 255 * red_rgb.x;
            unsigned int green            = 255 * red_rgb.y;
            unsigned int blue             = 255 * red_rgb.z;
            m_col_hovered_table[i]        = (alpha << 24) | (red << 16) | (green << 8) | blue;
        }
    }

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
                    DrawRingBufferStat(stat.ringbuf_stat);

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

    void BuiltinStatisticsWidget::DrawRingBufferStat(const RingUniformBufferStat& stat)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext&     g     = *GImGui;
        const ImGuiStyle& style = g.Style;

        ImGui::PushID(&stat);

        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;

        if (ImGui::TreeNodeEx("Ring Uniform Buffer", flag))
        {
            ImVec2 graph_size;
            graph_size.x = 0.8 * ImGui::GetWindowWidth() - 2.0 * style.FramePadding.x;
            graph_size.y = 2.0 * ImGui::GetTextLineHeight() + (style.FramePadding.y * 2);

            const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + graph_size);
            const ImRect inner_bb(frame_bb.Min + style.FramePadding,
                                  frame_bb.Max - style.FramePadding - ImVec2(0.0f, ImGui::GetTextLineHeight()));

            float inner_width = inner_bb.Max.x - inner_bb.Min.x;

            ImGui::ItemSize(frame_bb, style.FramePadding.y);
            if (!ImGui::ItemAdd(frame_bb, 0, &frame_bb))
                return;

            ImGui::RenderFrame(
                inner_bb.Min, inner_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

            float start_x_percent = (double)stat.begin / stat.size;
            float end_x_percent   = (double)(stat.begin + stat.usage) / stat.size;

            auto res_stat_pos0 = inner_bb.Min;
            auto res_stat_pos1 = inner_bb.Min + ImVec2(start_x_percent * inner_width, ImGui::GetTextLineHeight());
            auto cur_stat_pos0 = inner_bb.Min + ImVec2(start_x_percent * inner_width, 0);
            auto cur_stat_pos1 = inner_bb.Min + ImVec2(end_x_percent * inner_width, ImGui::GetTextLineHeight());

            int color_table_index = start_x_percent / k_gredint_partition;
            color_table_index     = std::clamp(color_table_index, 0, k_gredint_count - 1);
            window->DrawList->AddRectFilled(res_stat_pos0, res_stat_pos1, m_col_base_table[color_table_index]);
            window->DrawList->AddRect(res_stat_pos0, res_stat_pos1, col_outline);
            window->DrawList->AddRectFilled(cur_stat_pos0, cur_stat_pos1, m_col_hovered_table[color_table_index]);
            window->DrawList->AddRect(cur_stat_pos0, cur_stat_pos1, col_outline);

            ImGui::RenderText(inner_bb.Min + ImVec2(0.0, 1.5 * ImGui::GetTextLineHeight()), "0");
            ImGui::RenderText(inner_bb.Min + ImVec2(start_x_percent * inner_width, 1.5 * ImGui::GetTextLineHeight()),
                              std::to_string(stat.begin).c_str());
            ImGui::RenderText(inner_bb.Min + ImVec2(end_x_percent * inner_width, 1.5 * ImGui::GetTextLineHeight()),
                              std::to_string(stat.begin + stat.usage).c_str());
            ImGui::RenderText(inner_bb.Min + ImVec2(inner_width, 1.5 * ImGui::GetTextLineHeight()),
                              std::to_string(stat.size).c_str());

            ImGui::TreePop();
        }
        ImGui::PopID();
    }
} // namespace Meow