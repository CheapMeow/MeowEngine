#include "ring_buffer_widget.h"

#include "imgui_internal.h"

#include <glm/gtx/color_space.hpp>

namespace Meow
{
    RingBufferWidget::RingBufferWidget()
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

    void RingBufferWidget::Draw(const std::unordered_map<std::string, RingUniformBufferStat>& ringbuf_stat_map)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext&     g     = *GImGui;
        const ImGuiStyle& style = g.Style;

        ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;

        ImGui::PushID(&ringbuf_stat_map);

        if (ImGui::TreeNodeEx("Ring Uniform Buffer", flag))
        {
            for (const auto& pair : ringbuf_stat_map)
            {
                ImGui::PushID(&pair);

                if (ImGui::TreeNodeEx(pair.first.c_str(), flag))
                {
                    ImVec2 graph_size;
                    graph_size.x = ImGui::GetWindowWidth() - 2.0 * style.FramePadding.x;
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

                    float start_x_percent = (double)pair.second.cur_begin / pair.second.max_size;
                    float end_x_percent =
                        (double)(pair.second.cur_begin + pair.second.cur_usage) / pair.second.max_size;

                    auto res_stat_pos0 = inner_bb.Min;
                    auto res_stat_pos1 =
                        inner_bb.Min + ImVec2(start_x_percent * inner_width, ImGui::GetTextLineHeight());
                    auto cur_stat_pos0 = inner_bb.Min + ImVec2(start_x_percent * inner_width, 0);
                    auto cur_stat_pos1 = inner_bb.Min + ImVec2(end_x_percent * inner_width, ImGui::GetTextLineHeight());

                    int color_table_index = start_x_percent / k_gredint_partition;
                    color_table_index     = std::clamp(color_table_index, 0, k_gredint_count - 1);
                    window->DrawList->AddRectFilled(res_stat_pos0, res_stat_pos1, m_col_base_table[color_table_index]);
                    window->DrawList->AddRect(res_stat_pos0, res_stat_pos1, col_outline);
                    window->DrawList->AddRectFilled(
                        cur_stat_pos0, cur_stat_pos1, m_col_hovered_table[color_table_index]);
                    window->DrawList->AddRect(cur_stat_pos0, cur_stat_pos1, col_outline);

                    ImGui::RenderText(inner_bb.Min + ImVec2(0.0, 1.5 * ImGui::GetTextLineHeight()), "0");
                    ImGui::RenderText(inner_bb.Min +
                                          ImVec2(start_x_percent * inner_width, 1.5 * ImGui::GetTextLineHeight()),
                                      std::to_string(pair.second.cur_begin).c_str());
                    ImGui::RenderText(inner_bb.Min +
                                          ImVec2(end_x_percent * inner_width, 1.5 * ImGui::GetTextLineHeight()),
                                      std::to_string(pair.second.cur_begin + pair.second.cur_usage).c_str());

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }
} // namespace Meow