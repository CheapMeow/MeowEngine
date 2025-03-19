#include "deferred_pass.h"

#include "pch.h"

#include "core/math/math.h"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_context.h"
#include "function/object/game_object.h"
#include "function/render/buffer_data/per_scene_data.h"
#include "function/render/utils/model_utils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

namespace Meow
{
    void DeferredPass::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        auto obj_shader_ptr = std::make_shared<Shader>(
            physical_device, logical_device, "builtin/shaders/obj.vert.spv", "builtin/shaders/obj.frag.spv");

        m_obj2attachment_mat                        = Material(obj_shader_ptr);
        m_obj2attachment_mat.color_attachment_count = 3;
        m_obj2attachment_mat.CreatePipeline(logical_device, render_pass, vk::FrontFace::eClockwise, true);

        auto quad_shader_ptr = std::make_shared<Shader>(
            physical_device, logical_device, "builtin/shaders/quad.vert.spv", "builtin/shaders/quad.frag.spv");

        m_quad_mat         = Material(quad_shader_ptr);
        m_quad_mat.subpass = 1;
        m_quad_mat.CreatePipeline(logical_device, render_pass, vk::FrontFace::eClockwise, false);

        // Create quad model
        std::vector<float>    vertices = {-1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
                                          1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f};
        std::vector<uint32_t> indices  = {0, 1, 2, 0, 2, 3};

        m_quad_model =
            std::move(Model(std::move(vertices), std::move(indices), m_quad_mat.shader_ptr->per_vertex_attributes));

        input_vertex_attributes = m_obj2attachment_mat.shader_ptr->per_vertex_attributes;

        for (int32_t i = 0; i < k_num_lights; ++i)
        {
            m_LightDatas.lights[i].position.x = glm::linearRand<float>(-10.0f, 10.0f);
            m_LightDatas.lights[i].position.y = glm::linearRand<float>(-10.0f, 10.0f);
            m_LightDatas.lights[i].position.z = glm::linearRand<float>(-10.0f, 10.0f);

            m_LightDatas.lights[i].color.x = glm::linearRand<float>(0.0f, 1.0f);
            m_LightDatas.lights[i].color.y = glm::linearRand<float>(0.0f, 1.0f);
            m_LightDatas.lights[i].color.z = glm::linearRand<float>(0.0f, 1.0f);

            m_LightDatas.lights[i].radius = glm::linearRand<float>(0.0f, 5.0f);

            m_LightInfos.position[i]  = m_LightDatas.lights[i].position;
            m_LightInfos.direction[i] = m_LightInfos.position[i];
            m_LightInfos.direction[i] = normalize(m_LightInfos.direction[i]);
            m_LightInfos.speed[i]     = 1.0f + glm::linearRand<float>(1.0f, 2.0f);
        }

        // skybox

        auto skybox_shader_ptr = std::make_shared<Shader>(
            physical_device, logical_device, "builtin/shaders/skybox.vert.spv", "builtin/shaders/skybox.frag.spv");

        m_skybox_mat         = Material(skybox_shader_ptr);
        m_skybox_mat.subpass = 2;
        m_skybox_mat.CreatePipeline(logical_device, render_pass, vk::FrontFace::eCounterClockwise, true);

        {
            auto texture_ptr = ImageData::CreateCubemap({
                "builtin/textures/cubemap/skybox_specular_X+.hdr",
                "builtin/textures/cubemap/skybox_specular_X-.hdr",
                "builtin/textures/cubemap/skybox_specular_Z+.hdr",
                "builtin/textures/cubemap/skybox_specular_Z-.hdr",
                "builtin/textures/cubemap/skybox_specular_Y+.hdr",
                "builtin/textures/cubemap/skybox_specular_Y-.hdr",
            });
            if (texture_ptr)
            {
                g_runtime_context.resource_system->Register(texture_ptr);
                m_skybox_mat.BindImageToDescriptorSet("environmentMap", *texture_ptr);
            }
        }

        auto cube_vertices = GenerateCubeVertices();
        m_skybox_model =
            std::move(Model(cube_vertices, std::vector<uint32_t> {}, skybox_shader_ptr->per_vertex_attributes));
    }

    void DeferredPass::RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                           const vk::Extent2D&               extent)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // clear

        framebuffers.clear();

        m_color_attachment    = nullptr;
        m_normal_attachment   = nullptr;
        m_position_attachment = nullptr;
        m_depth_attachment    = nullptr;

        // Create attachment

        m_color_attachment = ImageData::CreateAttachment(m_color_format,
                                                         extent,
                                                         vk::ImageUsageFlagBits::eColorAttachment |
                                                             vk::ImageUsageFlagBits::eInputAttachment,
                                                         vk::ImageAspectFlagBits::eColor,
                                                         {},
                                                         false);

        m_normal_attachment = ImageData::CreateAttachment(vk::Format::eR8G8B8A8Unorm,
                                                          extent,
                                                          vk::ImageUsageFlagBits::eColorAttachment |
                                                              vk::ImageUsageFlagBits::eInputAttachment,
                                                          vk::ImageAspectFlagBits::eColor,
                                                          {},
                                                          false);

        m_position_attachment = ImageData::CreateAttachment(vk::Format::eR16G16B16A16Sfloat,
                                                            extent,
                                                            vk::ImageUsageFlagBits::eColorAttachment |
                                                                vk::ImageUsageFlagBits::eInputAttachment,
                                                            vk::ImageAspectFlagBits::eColor,
                                                            {},
                                                            false);

        m_depth_attachment = ImageData::CreateAttachment(m_depth_format,
                                                         extent,
                                                         vk::ImageUsageFlagBits::eDepthStencilAttachment |
                                                             vk::ImageUsageFlagBits::eInputAttachment,
                                                         vk::ImageAspectFlagBits::eDepth,
                                                         {},
                                                         false);

        // Provide attachment information to frame buffer

        vk::ImageView attachments[5];
        attachments[1] = *m_color_attachment->image_view;
        attachments[2] = *m_normal_attachment->image_view;
        attachments[3] = *m_position_attachment->image_view;
        attachments[4] = *m_depth_attachment->image_view;

        vk::FramebufferCreateInfo framebuffer_create_info(vk::FramebufferCreateFlags(), /* flags */
                                                          *render_pass,                 /* renderPass */
                                                          5,                            /* attachmentCount */
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

        // Update descriptor set

        m_quad_mat.BindImageToDescriptorSet("inputColor", *m_color_attachment);
        m_quad_mat.BindImageToDescriptorSet("inputNormal", *m_normal_attachment);
        m_quad_mat.BindImageToDescriptorSet("inputPosition", *m_position_attachment);
        m_quad_mat.BindImageToDescriptorSet("inputDepth", *m_depth_attachment);
    }

    void DeferredPass::UpdateUniformBuffer()
    {
        FUNCTION_TIMER();

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

        PerSceneData per_scene_data;
        per_scene_data.view = view;
        per_scene_data.projection =
            Math::perspective_vk(camera_comp_ptr->field_of_view,
                                 static_cast<float>(window_size[0]) / static_cast<float>(window_size[1]),
                                 camera_comp_ptr->near_plane,
                                 camera_comp_ptr->far_plane);

        m_obj2attachment_mat.PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));

        // Update mesh uniform

        m_obj2attachment_mat.BeginPopulatingDynamicUniformBufferPerFrame();
        const auto* visibles_forward_ptr = level_ptr->GetVisiblesPerMaterial(m_obj2attachment_mat.uuid);
        if (visibles_forward_ptr)
        {
            const auto& visibles_forward = *visibles_forward_ptr;
            for (const auto& visible : visibles_forward)
            {
                std::shared_ptr<GameObject>           gameobject_ptr = visible.lock();
                std::shared_ptr<Transform3DComponent> transfrom_comp_ptr2 =
                    gameobject_ptr->TryGetComponent<Transform3DComponent>("Transform3DComponent");
                std::shared_ptr<ModelComponent> model_comp_ptr =
                    gameobject_ptr->TryGetComponent<ModelComponent>("ModelComponent");

                if (!transfrom_comp_ptr2 || !model_comp_ptr)
                    continue;

#ifdef MEOW_DEBUG
                if (!gameobject_ptr)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!transfrom_comp_ptr2)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!model_comp_ptr)
                    MEOW_ERROR("shared ptr is invalid!");
#endif

                auto model = transfrom_comp_ptr2->GetTransform();

                for (uint32_t i = 0; i < model_comp_ptr->model_ptr.lock()->meshes.size(); ++i)
                {
                    m_obj2attachment_mat.BeginPopulatingDynamicUniformBufferPerObject();
                    m_obj2attachment_mat.PopulateDynamicUniformBuffer("objData", &model, sizeof(model));
                    m_obj2attachment_mat.EndPopulatingDynamicUniformBufferPerObject();
                }
            }
            m_obj2attachment_mat.EndPopulatingDynamicUniformBufferPerFrame();
        }

        // update light

        for (int32_t i = 0; i < k_num_lights; ++i)
        {
            float bias = glm::sin(g_runtime_context.time_system->GetTime() * m_LightInfos.speed[i]) / 5.0f;
            m_LightDatas.lights[i].position.x = m_LightInfos.position[i].x + bias * m_LightInfos.direction[i].x;
            m_LightDatas.lights[i].position.y = m_LightInfos.position[i].y + bias * m_LightInfos.direction[i].y;
            m_LightDatas.lights[i].position.z = m_LightInfos.position[i].z + bias * m_LightInfos.direction[i].z;
        }

        m_quad_mat.PopulateUniformBuffer("lightDatas", &m_LightDatas, sizeof(m_LightDatas));

        // skybox

        per_scene_data.view = lookAt(glm::vec3(0.0f), glm::vec3(0.0f) + forward, glm::vec3(0.0f, 1.0f, 0.0f));
        m_skybox_mat.PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));
    }

    void DeferredPass::Start(const vk::raii::CommandBuffer& command_buffer,
                             vk::Extent2D                   extent,
                             uint32_t                       current_image_index)
    {
        for (int i = 0; i < 2; i++)
        {
            draw_call[i] = 0;
        }

        RenderPass::Start(command_buffer, extent, current_image_index);
    }

    void DeferredPass::RenderGBuffer(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_obj2attachment_mat.BindDescriptorSetToPipeline(command_buffer, 0, 1);

        std::shared_ptr<Level> level_ptr            = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        const auto*            visibles_forward_ptr = level_ptr->GetVisiblesPerMaterial(m_obj2attachment_mat.uuid);
        if (visibles_forward_ptr)
        {
            const auto& visibles_forward = *visibles_forward_ptr;
            for (const auto& visible : visibles_forward)
            {
                std::shared_ptr<GameObject> gameobject_ptr = visible.lock();
                if (!gameobject_ptr)
                    continue;

                std::shared_ptr<ModelComponent> model_comp_ptr =
                    gameobject_ptr->TryGetComponent<ModelComponent>("ModelComponent");
                if (!model_comp_ptr)
                    continue;

                auto model_res_ptr = model_comp_ptr->model_ptr.lock();
                if (!model_res_ptr)
                    continue;

                for (uint32_t i = 0; i < model_res_ptr->meshes.size(); ++i)
                {
                    m_obj2attachment_mat.BindDescriptorSetToPipeline(command_buffer, 1, 1, draw_call[0], true);
                    model_res_ptr->meshes[i]->BindDrawCmd(command_buffer);

                    ++draw_call[0];
                }
            }
        }
    }

    void DeferredPass::MeshLighting(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_quad_mat.BindDescriptorSetToPipeline(command_buffer, 0, 1);
        for (int32_t i = 0; i < m_quad_model.meshes.size(); ++i)
        {
            m_quad_model.meshes[i]->BindDrawCmd(command_buffer);

            ++draw_call[1];
        }
    }

    void DeferredPass::RenderSkybox(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_skybox_mat.BindDescriptorSetToPipeline(command_buffer, 0, 2);

        m_skybox_model.meshes[0]->BindDrawCmd(command_buffer);
    }

    void swap(DeferredPass& lhs, DeferredPass& rhs)
    {
        using std::swap;

        swap(lhs.m_color_format, rhs.m_color_format);

        swap(lhs.m_obj2attachment_mat, rhs.m_obj2attachment_mat);
        swap(lhs.m_quad_mat, rhs.m_quad_mat);
        swap(lhs.m_quad_model, rhs.m_quad_model);
        swap(lhs.m_skybox_mat, rhs.m_skybox_mat);
        swap(lhs.m_skybox_model, rhs.m_skybox_model);

        swap(lhs.m_color_attachment, rhs.m_color_attachment);
        swap(lhs.m_normal_attachment, rhs.m_normal_attachment);
        swap(lhs.m_position_attachment, rhs.m_position_attachment);

        swap(lhs.m_LightDatas, rhs.m_LightDatas);
        swap(lhs.m_LightInfos, rhs.m_LightInfos);

        swap(lhs.m_pass_names, rhs.m_pass_names);
        swap(lhs.draw_call, rhs.draw_call);
    }
} // namespace Meow