#include "forward_pass.h"

#include "pch.h"

#include "core/math/math.h"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_context.h"
#include "function/render/structs/ubo_data.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    void ForwardPass::CreateMaterial(const vk::raii::PhysicalDevice& physical_device,
                                     const vk::raii::Device&         device,
                                     DescriptorAllocatorGrowable&    m_descriptor_allocator)
    {
        auto mesh_shader_ptr = std::make_shared<Shader>(physical_device,
                                                        device,
                                                        m_descriptor_allocator,
                                                        "builtin/shaders/mesh.vert.spv",
                                                        "builtin/shaders/mesh.frag.spv");

        m_forward_mat = Material(physical_device, device, mesh_shader_ptr);
        m_forward_mat.CreatePipeline(device, render_pass, vk::FrontFace::eClockwise, true);

        input_vertex_attributes = m_forward_mat.shader_ptr->per_vertex_attributes;

        m_dynamic_uniform_buffer = std::make_shared<UniformBuffer>(physical_device,
                                                                   device,
                                                                   32 * 1024,
                                                                   vk::BufferUsageFlagBits::eUniformBuffer |
                                                                       vk::BufferUsageFlagBits::eTransferDst);

        m_forward_mat.GetShader()->BindBufferToDescriptor(device, "uboMVP", m_dynamic_uniform_buffer->buffer);
    }

    void ForwardPass::RefreshFrameBuffers(const vk::raii::PhysicalDevice&   physical_device,
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

    void ForwardPass::UpdateUniformBuffer()
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
        glm::mat4 view =
            lookAt(transfrom_comp_ptr->position, transfrom_comp_ptr->position + forward, glm::vec3(0.0f, 1.0f, 0.0f));

        ubo_data.view = view;
        ubo_data.projection =
            Math::perspective_vk(camera_comp_ptr->field_of_view,
                                 static_cast<float>(window_size[0]) / static_cast<float>(window_size[1]),
                                 camera_comp_ptr->near_plane,
                                 camera_comp_ptr->far_plane);

        // Update mesh uniform

        m_dynamic_uniform_buffer->Reset();
        m_forward_mat.BeginPopulatingDynamicUniformBufferPerFrame();
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
            ubo_data.model = rotate(ubo_data.model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            for (int32_t i = 0; i < model_comp_ptr->model_ptr.lock()->meshes.size(); ++i)
            {
                m_forward_mat.BeginPopulatingDynamicUniformBufferPerObject();
                m_forward_mat.PopulateDynamicUniformBuffer(
                    m_dynamic_uniform_buffer, "uboMVP", &ubo_data, sizeof(ubo_data));
                m_forward_mat.EndPopulatingDynamicUniformBufferPerObject();
            }
        }
        m_forward_mat.EndPopulatingDynamicUniformBufferPerFrame();
    }

    void
    ForwardPass::Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t current_image_index)
    {
        draw_call = 0;

        RenderPass::Start(command_buffer, extent, current_image_index);
    }

    void ForwardPass::DrawOnly(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

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
                m_forward_mat.UpdateDynamicUniformPerObject(command_buffer, "uboMVP", draw_call);
                model_comp_ptr->model_ptr.lock()->meshes[i]->BindDrawCmd(command_buffer);

                ++draw_call;
            }
        }
    }

    void swap(ForwardPass& lhs, ForwardPass& rhs)
    {
        using std::swap;

        swap(lhs.m_forward_mat, rhs.m_forward_mat);
        swap(lhs.m_dynamic_uniform_buffer, rhs.m_dynamic_uniform_buffer);

        swap(lhs.draw_call, rhs.draw_call);
    }
} // namespace Meow
