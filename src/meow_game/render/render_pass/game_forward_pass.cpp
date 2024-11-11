#include "game_forward_pass.h"

#include "pch.h"

#include "core/math/math.h"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_context.h"
#include "function/render/structs/ubo_data.h"

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

namespace Meow
{
    GameForwardPass::GameForwardPass(const vk::raii::PhysicalDevice& physical_device,
                                     const vk::raii::Device&         device,
                                     SurfaceData&                    surface_data,
                                     const vk::raii::CommandPool&    command_pool,
                                     const vk::raii::Queue&          queue,
                                     DescriptorAllocatorGrowable&    m_descriptor_allocator)
        : RenderPass(device)
    {
        m_pass_name = "Forward Pass";

        // Create a set to store all information of attachments

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        std::vector<vk::AttachmentDescription> attachment_descriptions;
        // swap chain attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                             /* flags */
                                             color_format,
                                             /* format */
                                             m_sample_count,
                                             /* samples */
                                             vk::AttachmentLoadOp::eClear,
                                             /* loadOp */
                                             vk::AttachmentStoreOp::eStore,
                                             /* storeOp */
                                             vk::AttachmentLoadOp::eDontCare,
                                             /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eDontCare,
                                             /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,
                                             /* initialLayout */
                                             vk::ImageLayout::ePresentSrcKHR); /* finalLayout */
        // depth attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                             /* flags */
                                             m_depth_format,
                                             /* format */
                                             m_sample_count,
                                             /* samples */
                                             vk::AttachmentLoadOp::eClear,
                                             /* loadOp */
                                             vk::AttachmentStoreOp::eStore,
                                             /* storeOp */
                                             vk::AttachmentLoadOp::eClear,
                                             /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eStore,
                                             /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,
                                             /* initialLayout */
                                             vk::ImageLayout::eDepthStencilAttachmentOptimal); /* finalLayout */

        // Create reference to attachment information set

        vk::AttachmentReference swapchain_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference depth_attachment_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        // Create subpass

        std::vector<vk::SubpassDescription> subpass_descriptions;
        // obj2attachment pass
        subpass_descriptions.push_back(vk::SubpassDescription(vk::SubpassDescriptionFlags(),
                                                              /* flags */
                                                              vk::PipelineBindPoint::eGraphics,
                                                              /* pipelineBindPoint */
                                                              {},
                                                              /* pInputAttachments */
                                                              swapchain_attachment_reference,
                                                              /* pColorAttachments */
                                                              {},
                                                              /* pResolveAttachments */
                                                              &depth_attachment_reference,
                                                              /* pDepthStencilAttachment */
                                                              nullptr)); /* pPreserveAttachments */

        // Create subpass dependency

        std::vector<vk::SubpassDependency> dependencies;
        // externel -> forward pass
        dependencies.emplace_back(VK_SUBPASS_EXTERNAL,
                                  /* srcSubpass */
                                  0,
                                  /* dstSubpass */
                                  vk::PipelineStageFlagBits::eBottomOfPipe,
                                  /* srcStageMask */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  /* dstStageMask */
                                  vk::AccessFlagBits::eMemoryRead,
                                  /* srcAccessMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead,
                                  /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion); /* dependencyFlags */
        // forward -> externel
        dependencies.emplace_back(0,
                                  /* srcSubpass */
                                  VK_SUBPASS_EXTERNAL,
                                  /* dstSubpass */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  /* srcStageMask */
                                  vk::PipelineStageFlagBits::eBottomOfPipe,
                                  /* dstStageMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead,
                                  /* srcAccessMask */
                                  vk::AccessFlagBits::eMemoryRead,
                                  /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion); /* dependencyFlags */

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(),
                                                         /* flags */
                                                         attachment_descriptions,
                                                         /* pAttachments */
                                                         subpass_descriptions,
                                                         /* pSubpasses */
                                                         dependencies); /* pDependencies */

        render_pass = vk::raii::RenderPass(device, render_pass_create_info);

        // Create Material

        auto mesh_shader_ptr = std::make_shared<Shader>(physical_device,
                                                        device,
                                                        m_descriptor_allocator,
                                                        "builtin/shaders/mesh.vert.spv",
                                                        "builtin/shaders/mesh.frag.spv");

        m_forward_mat = Material(physical_device, device, mesh_shader_ptr);
        m_forward_mat.CreatePipeline(device, render_pass, vk::FrontFace::eClockwise, true);

        input_vertex_attributes = m_forward_mat.shader_ptr->per_vertex_attributes;

        clear_values.resize(2);
        clear_values[0].color        = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 0.2f);
        clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
    }

    void GameForwardPass::RefreshFrameBuffers(const vk::raii::PhysicalDevice&   physical_device,
                                              const vk::raii::Device&           device,
                                              const vk::raii::CommandPool&      command_pool,
                                              const vk::raii::Queue&            queue,
                                              const std::vector<vk::ImageView>& output_image_views,
                                              const vk::Extent2D&               extent)
    {
        // clear

        framebuffers.clear();

        m_depth_attachment = nullptr;

        // Create attachment

        m_depth_attachment = ImageData::CreateAttachment(physical_device,
                                                         device,
                                                         command_pool,
                                                         queue,
                                                         m_depth_format,
                                                         extent,
                                                         vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                         vk::ImageAspectFlagBits::eDepth,
                                                         {},
                                                         false);

        // Provide attachment information to frame buffer

        vk::ImageView attachments[2];
        attachments[1] = *m_depth_attachment->image_view;

        vk::FramebufferCreateInfo framebuffer_create_info(vk::FramebufferCreateFlags(), /* flags */
                                                          *render_pass,                 /* renderPass */
                                                          2,                            /* attachmentCount */
                                                          attachments,                  /* pAttachments */
                                                          extent.width,                 /* width */
                                                          extent.height,                /* height */
                                                          1);                           /* layers */

        framebuffers.reserve(output_image_views.size());
        for (const auto& imageView : output_image_views)
        {
            attachments[0] = imageView;
            framebuffers.push_back(vk::raii::Framebuffer(device, framebuffer_create_info));
        }
    }

    void GameForwardPass::UpdateUniformBuffer()
    {
        FUNCTION_TIMER();

        UBOData ubo_data;

        std::shared_ptr<Level> level_ptr = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

#ifdef MEOW_DEBUG
        if (!level_ptr)
            MEOW_ERROR("shared ptr is invalid!");
#endif

        std::shared_ptr<GameObject> camera_go_ptr = level_ptr->GetGameObjectByID(level_ptr->GetMainCameraID()).lock();
        std::shared_ptr<Transform3DComponent> transfrom_comp_ptr =
            camera_go_ptr->TryGetComponent<Transform3DComponent>("Transform3DComponent");
        std::shared_ptr<Camera3DComponent> camera_comp_ptr =
            camera_go_ptr->TryGetComponent<Camera3DComponent>("Camera3DComponent");

#ifdef MEOW_DEBUG
        if (!camera_go_ptr)
            MEOW_ERROR("shared ptr is invalid!");
        if (!transfrom_comp_ptr)
            MEOW_ERROR("shared ptr is invalid!");
        if (!camera_comp_ptr)
            MEOW_ERROR("shared ptr is invalid!");
#endif

        glm::ivec2 window_size = g_runtime_context.window_system->GetCurrentFocusWindow()->GetSize();

        glm::vec3 forward = transfrom_comp_ptr->rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        glm::mat4 view    = glm::lookAt(
            transfrom_comp_ptr->position, transfrom_comp_ptr->position + forward, glm::vec3(0.0f, 1.0f, 0.0f));

        ubo_data.view       = view;
        ubo_data.projection = Math::perspective_vk(camera_comp_ptr->field_of_view,
                                                   (float)window_size[0] / (float)window_size[1],
                                                   camera_comp_ptr->near_plane,
                                                   camera_comp_ptr->far_plane);

        // Update mesh uniform

        m_forward_mat.BeginFrame();
        const auto& all_gameobjects_map = level_ptr->GetAllVisibles();
        for (const auto& kv : all_gameobjects_map)
        {
            std::shared_ptr<GameObject>           model_go_ptr = kv.second.lock();
            std::shared_ptr<Transform3DComponent> transfrom_comp_ptr2 =
                model_go_ptr->TryGetComponent<Transform3DComponent>("Transform3DComponent");
            std::shared_ptr<ModelComponent> model_comp_ptr =
                model_go_ptr->TryGetComponent<ModelComponent>("ModelComponent");

            if (!transfrom_comp_ptr2 || !model_comp_ptr)
                continue;

#ifdef MEOW_DEBUG
            if (!model_go_ptr)
                MEOW_ERROR("shared ptr is invalid!");
            if (!transfrom_comp_ptr2)
                MEOW_ERROR("shared ptr is invalid!");
            if (!model_comp_ptr)
                MEOW_ERROR("shared ptr is invalid!");
#endif

            ubo_data.model = transfrom_comp_ptr2->GetTransform();
            ubo_data.model = glm::rotate(ubo_data.model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            for (int32_t i = 0; i < model_comp_ptr->model_ptr.lock()->meshes.size(); ++i)
            {
                m_forward_mat.BeginObject();
                m_forward_mat.SetLocalUniformBuffer("uboMVP", &ubo_data, sizeof(ubo_data));
                m_forward_mat.EndObject();
            }
        }
        m_forward_mat.EndFrame();
    }

    void GameForwardPass::Start(const vk::raii::CommandBuffer& command_buffer,
                                vk::Extent2D                   extent,
                                uint32_t                       current_image_index)
    {
        RenderPass::Start(command_buffer, extent, current_image_index);

        draw_call = 0;
    }

    void GameForwardPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_forward_mat.BindPipeline(command_buffer);

        std::shared_ptr<Level> level_ptr           = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        const auto&            all_gameobjects_map = level_ptr->GetAllVisibles();
        for (const auto& kv : all_gameobjects_map)
        {
            std::shared_ptr<GameObject>     model_go_ptr = kv.second.lock();
            std::shared_ptr<ModelComponent> model_comp_ptr =
                model_go_ptr->TryGetComponent<ModelComponent>("ModelComponent");

            if (!model_comp_ptr)
                continue;

            for (int32_t i = 0; i < model_comp_ptr->model_ptr.lock()->meshes.size(); ++i)
            {
                m_forward_mat.BindDescriptorSets(command_buffer, draw_call);
                model_comp_ptr->model_ptr.lock()->meshes[i]->BindDrawCmd(command_buffer);

                ++draw_call;
            }
        }
    }

    void GameForwardPass::AfterPresent() {}

    void swap(GameForwardPass& lhs, GameForwardPass& rhs)
    {
        using std::swap;

        swap(lhs.m_forward_mat, rhs.m_forward_mat);
    }
} // namespace Meow