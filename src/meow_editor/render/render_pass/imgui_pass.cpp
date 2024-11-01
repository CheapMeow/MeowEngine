#include "imgui_pass.h"

#include "pch.h"

#include "global/editor_context.h"
#include "meow_runtime/function/global/runtime_context.h"
#include "render/imgui_widgets/pipeline_statistics_widget.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

namespace Meow
{
    ImGuiPass::ImGuiPass(std::nullptr_t)
        : RenderPass(nullptr)
    {}

    ImGuiPass::ImGuiPass(vk::raii::PhysicalDevice const& physical_device,
                         vk::raii::Device const&         device,
                         SurfaceData&                    surface_data,
                         vk::raii::CommandPool const&    command_pool,
                         vk::raii::Queue const&          queue,
                         DescriptorAllocatorGrowable&    m_descriptor_allocator)
        : RenderPass(device)
    {
        m_pass_name = "ImGui Pass";

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        vk::AttachmentReference swapchain_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

        // swap chain attachment
        vk::AttachmentDescription attachment_description(vk::AttachmentDescriptionFlags(), /* flags */
                                                         color_format,                     /* format */
                                                         vk::SampleCountFlagBits::e1,      /* samples */
                                                         vk::AttachmentLoadOp::eLoad,      /* loadOp */
                                                         vk::AttachmentStoreOp::eStore,    /* storeOp */
                                                         vk::AttachmentLoadOp::eDontCare,  /* stencilLoadOp */
                                                         vk::AttachmentStoreOp::eDontCare, /* stencilStoreOp */
                                                         vk::ImageLayout::ePresentSrcKHR,  /* initialLayout */
                                                         vk::ImageLayout::ePresentSrcKHR); /* finalLayout */

        vk::SubpassDescription subpass_description(
            vk::SubpassDescription(vk::SubpassDescriptionFlags(),    /* flags */
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

        render_pass = vk::raii::RenderPass(device, render_pass_create_info);

        // loadOp is load, clear value doesn't matter
        clear_values.resize(1);
        clear_values[0].color = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
    }

    void ImGuiPass::RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                        vk::raii::Device const&                 device,
                                        vk::raii::CommandPool const&            command_pool,
                                        vk::raii::Queue const&                  queue,
                                        SurfaceData&                            surface_data,
                                        std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                        vk::Extent2D const&                     extent)
    {
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

        framebuffers.reserve(swapchain_image_views.size());
        for (auto const& imageView : swapchain_image_views)
        {
            attachments[0] = *imageView;
            framebuffers.push_back(vk::raii::Framebuffer(device, framebuffer_create_info));
        }
    }

    void ImGuiPass::Start(vk::raii::CommandBuffer const& command_buffer,
                          Meow::SurfaceData const&       surface_data,
                          uint32_t                       current_image_index)
    {
        FUNCTION_TIMER();

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);

        ImGui::Begin("Switch RenderPass");
        if (ImGui::Combo(
                "Current Render Pass", &m_cur_render_pass, m_render_pass_names.data(), m_render_pass_names.size()))
            m_on_pass_changed(m_cur_render_pass);
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        std::shared_ptr<Level> level_ptr = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

#ifdef MEOW_DEBUG
        if (!level_ptr)
            MEOW_ERROR("shared ptr is invalid!");
#endif

        const auto& all_gameobjects_map = level_ptr->GetAllGameObjects();

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

        RenderPass::Start(command_buffer, surface_data, current_image_index);
    }

    void ImGuiPass::Draw(vk::raii::CommandBuffer const& command_buffer)
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

    void swap(ImGuiPass& lhs, ImGuiPass& rhs)
    {
        using std::swap;

        swap(lhs.m_components_widget, rhs.m_components_widget);
        swap(lhs.m_flame_graph_widget, rhs.m_flame_graph_widget);

        swap(lhs.m_query_enabled, rhs.m_query_enabled);
        swap(lhs.query_pool, rhs.query_pool);
    }

} // namespace Meow
