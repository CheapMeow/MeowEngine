#include "imgui_pass.h"

#include "pch.h"

#include "global/editor_context.h"
#include "meow_runtime/function/global/runtime_context.h"
#include "render/imgui_widgets/pipeline_statistics_widget.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_internal.h>

namespace Meow
{
    ImGuiPass::ImGuiPass(std::nullptr_t)
        : RenderPass(nullptr)
    {}

    ImGuiPass::ImGuiPass(SurfaceData& surface_data)
        : RenderPass()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        vk::AttachmentReference swapchain_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

        // swap chain attachment
        vk::AttachmentDescription attachment_description(vk::AttachmentDescriptionFlags(), /* flags */
                                                         color_format,                     /* format */
                                                         vk::SampleCountFlagBits::e1,      /* samples */
                                                         vk::AttachmentLoadOp::eClear,     /* loadOp */
                                                         vk::AttachmentStoreOp::eStore,    /* storeOp */
                                                         vk::AttachmentLoadOp::eDontCare,  /* stencilLoadOp */
                                                         vk::AttachmentStoreOp::eDontCare, /* stencilStoreOp */
                                                         vk::ImageLayout::eUndefined,      /* initialLayout */
                                                         vk::ImageLayout::ePresentSrcKHR); /* finalLayout */

        auto subpass_description(vk::SubpassDescription(vk::SubpassDescriptionFlags(),    /* flags */
                                                        vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                                                        {},                               /* pInputAttachments */
                                                        swapchain_attachment_reference,   /* pColorAttachments */
                                                        {},                               /* pResolveAttachments */
                                                        {},                               /* pDepthStencilAttachment */
                                                        nullptr));                        /* pPreserveAttachments */

        vk::SubpassDependency dependencies(VK_SUBPASS_EXTERNAL,                               /* srcSubpass */
                                           0,                                                 /* dstSubpass */
                                           vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                                           vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dstStageMask */
                                           {},                                                /* srcAccessMask */
                                           vk::AccessFlagBits::eColorAttachmentWrite,         /* dstAccessMask */
                                           {});                                               /* dependencyFlags */

        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(), /* flags */
                                                         attachment_description,      /* pAttachments */
                                                         subpass_description,         /* pSubpasses */
                                                         dependencies);               /* pDependencies */

        render_pass = vk::raii::RenderPass(logical_device, render_pass_create_info);

        // loadOp is load, clear value doesn't matter
        clear_values.resize(1);
        clear_values[0].color = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
    }

    void ImGuiPass::RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                        const vk::Extent2D&               extent)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // clear

        framebuffers.clear();

        // Provide attachment information to frame buffer

        vk::ImageView attachments[1];

        vk::FramebufferCreateInfo framebuffer_create_info(vk::FramebufferCreateFlags(), /* flags */
                                                          *render_pass,                 /* renderPass */
                                                          1,                            /* attachmentCount */
                                                          attachments,                  /* pAttachments */
                                                          extent.width,                 /* width */
                                                          extent.height,                /* height */
                                                          1);                           /* layers */

        framebuffers.reserve(output_image_views.size());
        for (const auto& imageView : output_image_views)
        {
            attachments[0] = imageView;
            framebuffers.push_back(vk::raii::Framebuffer(logical_device, framebuffer_create_info));
        }
    }

    void
    ImGuiPass::Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t current_image_index)
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

        m_builtin_stat_widget.Draw(g_editor_context.profile_system->GetBuiltinRenderStat());

        if (m_query_enabled)
            PipelineStatisticsWidget::Draw(g_editor_context.profile_system->GetPipelineStat());
        else
            ImGui::Text("Pipeline Statistics is disabled.");

        ImGui::End();

        RenderPass::Start(command_buffer, extent, current_image_index);
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

        swap(lhs.m_cur_render_pass, rhs.m_cur_render_pass);
        swap(lhs.m_render_pass_names, rhs.m_render_pass_names);
        swap(lhs.m_on_pass_changed, rhs.m_on_pass_changed);

        swap(lhs.m_is_offscreen_image_valid, rhs.m_is_offscreen_image_valid);
        swap(lhs.m_offscreen_image_desc, rhs.m_offscreen_image_desc);

        swap(lhs.m_gameobjects_widget, rhs.m_gameobjects_widget);
        swap(lhs.m_components_widget, rhs.m_components_widget);
        swap(lhs.m_flame_graph_widget, rhs.m_flame_graph_widget);
        swap(lhs.m_builtin_stat_widget, rhs.m_builtin_stat_widget);

        swap(lhs.m_query_enabled, rhs.m_query_enabled);
        swap(lhs.query_pool, rhs.query_pool);
    }

} // namespace Meow
