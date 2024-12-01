#include "imgui_pass.h"

#include "pch.h"

#include "function/global/runtime_context.h"
#include "function/render/imgui_widgets/pipeline_statistics_widget.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_internal.h>

namespace Meow
{
    void ImGuiPass::DrawImGui()
    {
        FUNCTION_TIMER();

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);

        std::shared_ptr<Level> level_ptr = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

#ifdef MEOW_DEBUG
        if (!level_ptr)
            MEOW_ERROR("shared ptr is invalid!");
#endif

        static bool               p_open          = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags     window_flags = ImGuiWindowFlags_NoDocking;
        const ImGuiViewport* viewport     = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("DockSpace", &p_open, window_flags);

        ImGui::PopStyleVar(3);

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        ImGui::End();

        const auto& all_gameobjects_map = level_ptr->GetAllGameObjects();

        ImGui::Begin("Scene");
        const auto camera_id = level_ptr->GetMainCameraID();
        auto       it        = all_gameobjects_map.find(camera_id);
        if (it != all_gameobjects_map.end())
        {
            const auto camera_go_ptr   = it->second;
            const auto camera_comp_ptr = camera_go_ptr->TryGetComponent<Camera3DComponent>("Camera3DComponent");
            if (camera_comp_ptr)
            {
                auto   aspect_ratio = camera_comp_ptr->aspect_ratio; // width / height
                ImVec2 region_size  = ImGui::GetContentRegionAvail();
                float  width        = static_cast<float>(region_size.x);
                float  height       = static_cast<float>(region_size.y);
                if (width / height > aspect_ratio)
                {
                    width = aspect_ratio * height;
                }
                else
                {
                    height = width / aspect_ratio;
                }
                ImVec2 image_size = {width, height};
                ImGui::SetCursorPos((ImGui::GetWindowSize() - image_size) * 0.5f);
                ImGui::Image((ImTextureID)m_offscreen_image_desc, image_size);
            }
        }
        ImGui::End();

        ImGui::Begin("Switch RenderPass");
        if (ImGui::Combo(
                "Current Render Pass", &m_cur_render_pass, m_render_pass_names.data(), m_render_pass_names.size()))
            m_on_pass_changed(m_cur_render_pass);
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Begin("GameObject");
        m_gameobjects_widget.Draw(all_gameobjects_map);
        ImGui::End();

        auto gameobject_ptr_iter = all_gameobjects_map.find(m_gameobjects_widget.GetSelectedID());
        if (gameobject_ptr_iter != all_gameobjects_map.end())
        {
            ImGui::Begin("Component");
            m_components_widget.CreateGameObjectUI(gameobject_ptr_iter->second);
            ImGui::End();
        }

        ImGui::Begin("Statistics");

        m_flame_graph_widget.Draw(TimerSingleton::Get().GetScopeTimes(),
                                  TimerSingleton::Get().GetMaxDepth(),
                                  TimerSingleton::Get().GetGlobalStart());

        m_builtin_stat_widget.Draw(g_runtime_context.profile_system->GetMaterialStat());

        PipelineStatisticsWidget::Draw(g_runtime_context.profile_system->GetPipelineStat());

        ImGui::End();
    }

    void ImGuiPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *command_buffer);

        // Specially for docking branch
        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void ImGuiPass::RefreshOffscreenRenderTarget(VkSampler     offscreen_image_sampler,
                                                 VkImageView   offscreen_image_view,
                                                 VkImageLayout offscreen_image_layout)
    {
        m_offscreen_image_desc =
            ImGui_ImplVulkan_AddTexture(offscreen_image_sampler, offscreen_image_view, offscreen_image_layout);
    }

    void swap(ImGuiPass& lhs, ImGuiPass& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPass&>(lhs), static_cast<RenderPass&>(rhs));

        swap(lhs.m_cur_render_pass, rhs.m_cur_render_pass);
        swap(lhs.m_render_pass_names, rhs.m_render_pass_names);
        swap(lhs.m_on_pass_changed, rhs.m_on_pass_changed);

        swap(lhs.m_offscreen_image_desc, rhs.m_offscreen_image_desc);

        swap(lhs.m_gameobjects_widget, rhs.m_gameobjects_widget);
        swap(lhs.m_components_widget, rhs.m_components_widget);
        swap(lhs.m_flame_graph_widget, rhs.m_flame_graph_widget);
        swap(lhs.m_builtin_stat_widget, rhs.m_builtin_stat_widget);
    }

} // namespace Meow
