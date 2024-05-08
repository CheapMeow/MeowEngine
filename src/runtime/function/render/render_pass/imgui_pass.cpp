#include "imgui_pass.h"

#include "function/global/runtime_global_context.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

namespace Meow
{
    ImguiPass::ImguiPass(vk::raii::PhysicalDevice const& physical_device,
                         vk::raii::Device const&         device,
                         SurfaceData&                    surface_data,
                         vk::raii::CommandPool const&    command_pool,
                         vk::raii::Queue const&          queue,
                         DescriptorAllocatorGrowable&    m_descriptor_allocator)
        : RenderPass(nullptr)
    {
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

    void ImguiPass::RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                        vk::raii::Device const&                 device,
                                        vk::raii::CommandBuffer const&          command_buffer,
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

    void ImguiPass::Start(vk::raii::CommandBuffer const& command_buffer,
                          Meow::SurfaceData const&       surface_data,
                          uint32_t                       current_frame_index)
    {
        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Meow Engine");

        ImGui::Combo("Current Render Pass",
                     &g_runtime_global_context.render_system->cur_render_pass,
                     g_runtime_global_context.render_system->render_pass_names.data(),
                     g_runtime_global_context.render_system->render_pass_names.size());

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("invocationCount_fs = %d", invocationCount_fs_query_count);

        ImGui::End();

        RenderPass::Start(command_buffer, surface_data, current_frame_index);
    }

    void ImguiPass::Draw(vk::raii::CommandBuffer const& command_buffer)
    {
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
} // namespace Meow
