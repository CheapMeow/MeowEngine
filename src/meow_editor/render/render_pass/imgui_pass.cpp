#include "imgui_pass.h"

#include "pch.h"

#include "function/render/utils/vulkan_debug_utils.h"
#include "global/editor_context.h"
#include "meow_runtime/function/global/runtime_context.h"
#include "render/imgui_widgets/pipeline_statistics_widget.h"

#include <ImGuizmo.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <volk.h>

namespace Meow
{
    ImGuiPass::ImGuiPass(std::nullptr_t)
        : RenderPassBase(nullptr)
    {}

    ImGuiPass::ImGuiPass(SurfaceData& surface_data)
        : RenderPassBase(surface_data)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        vk::AttachmentReference swapchain_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

        // swap chain attachment
        vk::AttachmentDescription attachment_description(vk::AttachmentDescriptionFlags(), /* flags */
                                                         m_color_format,                   /* format */
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

    void ImGuiPass::Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index)
    {
        FUNCTION_TIMER();

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);

        std::shared_ptr<Level> level = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

        if (!level)
            MEOW_ERROR("shared ptr is invalid!");

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

        const auto& all_gameobjects_map = level->GetAllGameObjects();

        ImGui::Begin("Scene");
        const auto camera_id = level->GetMainCameraID();
        auto       it        = all_gameobjects_map.find(camera_id);
        if (it != all_gameobjects_map.end())
        {
            const auto main_camera           = it->second;
            const auto main_camera_component = main_camera->TryGetComponent<Camera3DComponent>("Camera3DComponent");
            if (main_camera_component)
            {
                auto   aspect_ratio = main_camera_component->aspect_ratio; // width / height
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
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                draw_list->AddCallback(
                    [](const ImDrawList* parent_list, const ImDrawCmd* cmd) {
                        vk::raii::CommandBuffer* command_buffer_ptr =
                            static_cast<vk::raii::CommandBuffer*>(cmd->UserCallbackData);
                        if (command_buffer_ptr)
                        {
                            ImGui_ImplVulkan_SwitchToNoColorBlendPipeline(**command_buffer_ptr);
                        }
                    },
                    reinterpret_cast<void*>(const_cast<vk::raii::CommandBuffer*>(&command_buffer)));
                ImGui::Image((ImTextureID)m_offscreen_image_descs[image_index], image_size);
                draw_list->AddCallback(
                    [](const ImDrawList* parent_list, const ImDrawCmd* cmd) {
                        vk::raii::CommandBuffer* command_buffer_ptr =
                            static_cast<vk::raii::CommandBuffer*>(cmd->UserCallbackData);
                        if (command_buffer_ptr)
                        {
                            ImGui_ImplVulkan_SwitchToDefaultPipeline(**command_buffer_ptr);
                        }
                    },
                    reinterpret_cast<void*>(const_cast<vk::raii::CommandBuffer*>(&command_buffer)));
            }
        }

        m_gizmo_widget.ShowGameObjectGizmo(m_gameobjects_widget.GetSelectedID());

        ImGui::End();

        {
            ImGui::Begin("Shadow Map");

            ImVec2 region_size = ImGui::GetContentRegionAvail();
            float  width       = static_cast<float>(region_size.x);
            float  height      = static_cast<float>(region_size.y);
            ImVec2 image_size  = {width, height};

            ImGui::Image((ImTextureID)m_shadow_map_desc, image_size);
            ImGui::End();
        }

        {
            ImGui::Begin("Shadow Coord");

            if (it != all_gameobjects_map.end())
            {
                const auto main_camera           = it->second;
                const auto main_camera_component = main_camera->TryGetComponent<Camera3DComponent>("Camera3DComponent");
                if (main_camera_component)
                {
                    auto   aspect_ratio = main_camera_component->aspect_ratio; // width / height
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
                    ImGui::Image((ImTextureID)m_shadow_coord_desc, image_size);
                }
            }

            ImGui::End();
        }

        {
            ImGui::Begin("Shadow Depth");

            if (it != all_gameobjects_map.end())
            {
                const auto main_camera           = it->second;
                const auto main_camera_component = main_camera->TryGetComponent<Camera3DComponent>("Camera3DComponent");
                if (main_camera_component)
                {
                    auto   aspect_ratio = main_camera_component->aspect_ratio; // width / height
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
                    ImGui::Image((ImTextureID)m_shadow_depth_desc, image_size);
                }
            }

            ImGui::End();
        }

        ImGui::Begin("Render Settings");
        if (ImGui::Combo(
                "Current Render Pass", &m_cur_render_pass, m_render_pass_names.data(), m_render_pass_names.size()))
        {
            m_on_pass_changed(m_cur_render_pass);
        }
        if (ImGui::Checkbox("MSAA Enabled", &m_msaa_enabled))
        {
            m_on_msaa_enabled_changed(m_msaa_enabled);
        }
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Begin("GameObject");
        m_gameobjects_widget.Draw(all_gameobjects_map);
        ImGui::End();

        auto current_gameobject_iter = all_gameobjects_map.find(m_gameobjects_widget.GetSelectedID());
        if (current_gameobject_iter != all_gameobjects_map.end())
        {
            ImGui::Begin("Component");
            m_components_widget.CreateGameObjectUI(current_gameobject_iter->second);
            ImGui::End();
        }

        ImGui::Begin("Statistics");

        m_flame_graph_widget.Draw(TimerSingleton::Get().GetScopeTimes(),
                                  TimerSingleton::Get().GetMaxDepth(),
                                  TimerSingleton::Get().GetGlobalStart());

        m_builtin_stat_widget.Draw(g_editor_context.profile_system->GetBuiltinRenderStat());

        PipelineStatisticsWidget::Draw(g_editor_context.profile_system->GetPipelineStat());

        ImGui::End();

        RenderPassBase::Start(command_buffer, extent, image_index);
    }

    void ImGuiPass::RecordGraphicsCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index)
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

    void ImGuiPass::RefreshOffscreenRenderTarget(std::vector<std::shared_ptr<ImageData>>& offscreen_render_targets,
                                                 VkImageLayout                            image_layout)
    {
        m_offscreen_image_descs.resize(offscreen_render_targets.size());
        for (uint32_t i = 0; i < offscreen_render_targets.size(); i++)
        {
            m_offscreen_image_descs[i] = ImGui_ImplVulkan_AddTexture(
                *offscreen_render_targets[i]->sampler, *offscreen_render_targets[i]->image_view, image_layout);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            const vk::raii::Device&         logical_device = g_runtime_context.render_system->GetLogicalDevice();
            std::string                     debug_name     = "Offscreen Image Descriptor Set " + std::to_string(i);
            vk::DebugUtilsObjectNameInfoEXT name_info      = {
                vk::ObjectType::eDescriptorSet,
                NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkDescriptorSet, m_offscreen_image_descs[i]),
                debug_name.c_str()};
            logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
        }
    }

    void ImGuiPass::RefreshShadowMap(VkSampler image_sampler, VkImageView image_view, VkImageLayout image_layout)
    {
        m_shadow_map_desc = ImGui_ImplVulkan_AddTexture(image_sampler, image_view, image_layout);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        const vk::raii::Device&         logical_device = g_runtime_context.render_system->GetLogicalDevice();
        vk::DebugUtilsObjectNameInfoEXT name_info      = {
            vk::ObjectType::eDescriptorSet,
            NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkDescriptorSet, m_shadow_map_desc),
            "Shadow Map to Color Render Target Descriptor Set"};
        logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void ImGuiPass::RefreshShadowCoord(VkSampler image_sampler, VkImageView image_view, VkImageLayout image_layout)
    {
        m_shadow_coord_desc = ImGui_ImplVulkan_AddTexture(image_sampler, image_view, image_layout);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        const vk::raii::Device&         logical_device = g_runtime_context.render_system->GetLogicalDevice();
        vk::DebugUtilsObjectNameInfoEXT name_info      = {
            vk::ObjectType::eDescriptorSet,
            NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkDescriptorSet, m_shadow_map_desc),
            "Shadow Coord to Color Render Target Descriptor Set"};
        logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void ImGuiPass::RefreshShadowDepth(VkSampler image_sampler, VkImageView image_view, VkImageLayout image_layout)
    {
        m_shadow_depth_desc = ImGui_ImplVulkan_AddTexture(image_sampler, image_view, image_layout);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        const vk::raii::Device&         logical_device = g_runtime_context.render_system->GetLogicalDevice();
        vk::DebugUtilsObjectNameInfoEXT name_info      = {
            vk::ObjectType::eDescriptorSet,
            NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkDescriptorSet, m_shadow_map_desc),
            "Shadow Depth to Color Render Target Descriptor Set"};
        logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void swap(ImGuiPass& lhs, ImGuiPass& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPassBase&>(lhs), static_cast<RenderPassBase&>(rhs));

        swap(lhs.m_cur_render_pass, rhs.m_cur_render_pass);
        swap(lhs.m_render_pass_names, rhs.m_render_pass_names);
        swap(lhs.m_on_pass_changed, rhs.m_on_pass_changed);

        swap(lhs.m_is_offscreen_image_valid, rhs.m_is_offscreen_image_valid);
        swap(lhs.m_offscreen_image_descs, rhs.m_offscreen_image_descs);

        swap(lhs.m_gameobjects_widget, rhs.m_gameobjects_widget);
        swap(lhs.m_components_widget, rhs.m_components_widget);
        swap(lhs.m_flame_graph_widget, rhs.m_flame_graph_widget);
        swap(lhs.m_builtin_stat_widget, rhs.m_builtin_stat_widget);
    }

} // namespace Meow
