#include "flame_graph_widget.h"

#include "imgui_internal.h"

#include <glm/gtx/color_space.hpp>

namespace Meow
{
    void FlameGraphWidget::Draw(const std::vector<ScopeTimeData>& scope_times,
                                int                               max_depth,
                                std::chrono::microseconds         global_start,
                                ImVec2                            graph_size)
    {
        if (scope_times.size() == 0)
            return;

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGui::Text("Flame Graph");

        if (m_is_shapshot_enabled)
            Draw_impl(m_curr_shapshot.scope_times,
                      m_curr_shapshot.max_depth,
                      m_curr_shapshot.global_start,
                      m_curr_shapshot.graph_size);
        else
            Draw_impl(scope_times, max_depth, global_start, graph_size);

        if (ImGui::Button("Capture Snapshot"))
        {
            m_is_shapshot_enabled        = true;
            m_curr_shapshot.scope_times  = scope_times;
            m_curr_shapshot.max_depth    = max_depth;
            m_curr_shapshot.global_start = global_start;
            m_curr_shapshot.graph_size   = graph_size;
        }

        if (m_is_shapshot_enabled)
        {
            ImGui::SameLine();
            if (ImGui::Button("Leave Snapshot"))
            {
                m_is_shapshot_enabled = false;
            }
        }
    }

    void FlameGraphWidget::Draw_impl(const std::vector<ScopeTimeData>& scope_times,
                                     int                               max_depth,
                                     std::chrono::microseconds         global_start,
                                     ImVec2                            graph_size)
    {
        if (scope_times.size() == 0)
            return;

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext&     g     = *GImGui;
        const ImGuiStyle& style = g.Style;

        const auto   blockHeight = ImGui::GetTextLineHeight() + (style.FramePadding.y * 2);
        const ImVec2 label_size  = ImGui::CalcTextSize("Testing", NULL, true);
        if (graph_size.x == 0.0f)
            graph_size.x = ImGui::GetWindowWidth() - 2.0 * style.FramePadding.x;
        if (graph_size.y == 0.0f)
            graph_size.y = label_size.y + (style.FramePadding.y * 3) + blockHeight * (max_depth + 1);

        const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + graph_size);
        const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
        ImGui::ItemSize(frame_bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(frame_bb, 0, &frame_bb))
            return;

        ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

        std::vector<ImU32> col_base_table(max_depth + 1);
        for (int i = 0; i < max_depth + 1; i++)
        {
            float        saturation_ratio = static_cast<float>(i + 1) / (max_depth + 1);
            glm::vec3    red_rgb          = glm::rgbColor(glm::vec3(120.0, saturation_ratio, 0.5));
            unsigned int alpha            = 255;
            unsigned int red              = 255 * red_rgb.x;
            unsigned int green            = 255 * red_rgb.y;
            unsigned int blue             = 255 * red_rgb.z;
            col_base_table[i]             = (alpha << 24) | (red << 16) | (green << 8) | blue;
        }
        std::vector<ImU32> col_hovered_table(max_depth + 1);
        for (int i = 0; i < max_depth + 1; i++)
        {
            float        saturation_ratio = static_cast<float>(i + 1) / (max_depth + 1);
            glm::vec3    red_rgb          = glm::rgbColor(glm::vec3(120.0, saturation_ratio, 0.8));
            unsigned int alpha            = 255;
            unsigned int red              = 255 * red_rgb.x;
            unsigned int green            = 255 * red_rgb.y;
            unsigned int blue             = 255 * red_rgb.z;
            col_hovered_table[i]          = (alpha << 24) | (red << 16) | (green << 8) | blue;
        }
        const ImU32 col_outline_base    = 0xFFFFFFFF;
        const ImU32 col_outline_hovered = 0xFFFFFFFF;

        auto frame_time =
            scope_times[scope_times.size() - 1].start + scope_times[scope_times.size() - 1].duration - global_start;

        float inner_width = inner_bb.Max.x - inner_bb.Min.x;

        bool any_hovered = false;

        for (const auto& scope_time : scope_times)
        {
            const ImU32 col_base    = col_base_table[scope_time.depth];
            const ImU32 col_hovered = col_hovered_table[scope_time.depth];

            auto start_time = scope_time.start - global_start;

            float start_x_percent = (double)start_time.count() / frame_time.count();
            float end_x_percent   = start_x_percent + (double)scope_time.duration.count() / frame_time.count();

            float bottom_height = blockHeight * (max_depth - scope_time.depth + 1) - style.FramePadding.y;

            auto pos0 = inner_bb.Min + ImVec2(start_x_percent * inner_width, bottom_height);
            auto pos1 = inner_bb.Min + ImVec2(end_x_percent * inner_width, bottom_height + blockHeight);

            bool v_hovered = false;
            if (ImGui::IsMouseHoveringRect(pos0, pos1))
            {
                ImGui::SetTooltip("%s: %8.4gms", scope_time.name.c_str(), (double)scope_time.duration.count() / 1000.0);
                v_hovered   = true;
                any_hovered = v_hovered;
            }

            window->DrawList->AddRectFilled(pos0, pos1, v_hovered ? col_hovered : col_base);
            window->DrawList->AddRect(pos0, pos1, v_hovered ? col_outline_hovered : col_outline_base);
            auto textSize   = ImGui::CalcTextSize(scope_time.name.c_str());
            auto boxSize    = (pos1 - pos0);
            auto textOffset = ImVec2(0.0f, 0.0f);
            if (textSize.x < boxSize.x)
            {
                textOffset = ImVec2(0.5f, 0.5f) * (boxSize - textSize);
                ImGui::RenderText(pos0 + textOffset, scope_time.name.c_str());
            }
        }

        if (!any_hovered && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Total: %8.4g", frame_time.count() / 1000.0);
        }

        ImGui::Columns();
    }

} // namespace Meow