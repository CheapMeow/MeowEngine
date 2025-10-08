#include "deferred_pass_base.h"

#include "pch.h"

#include "core/math/math.h"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_context.h"
#include "function/object/game_object.h"
#include "function/render/buffer_data/per_scene_data.h"
#include "function/render/geometry/geometry_factory.h"
#include "function/render/material/material_factory.h"
#include "function/render/material/shader_factory.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

namespace Meow
{
    void DeferredPassBase::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        ShaderFactory   shader_factory;
        MaterialFactory material_factory;

        auto obj_shader = shader_factory.clear()
                              .SetVertexShader("builtin/shaders/obj.vert.spv")
                              .SetFragmentShader("builtin/shaders/obj.frag.spv")
                              .Create();

        m_obj2attachment_material = std::make_shared<Material>(obj_shader);
        g_runtime_context.resource_system->Register(m_obj2attachment_material);
        material_factory.Init(obj_shader.get(), vk::FrontFace::eClockwise);
        material_factory.SetOpaque(true, 3);
        material_factory.CreatePipeline(
            logical_device, render_pass, obj_shader.get(), m_obj2attachment_material.get(), 0);

        auto quad_shader = shader_factory.clear()
                               .SetVertexShader("builtin/shaders/quad.vert.spv")
                               .SetFragmentShader("builtin/shaders/quad.frag.spv")
                               .Create();

        m_quad_material = std::make_shared<Material>(quad_shader);
        g_runtime_context.resource_system->Register(m_quad_material);
        material_factory.Init(quad_shader.get(), vk::FrontFace::eClockwise);
        material_factory.SetOpaque(false, 1);
        material_factory.CreatePipeline(logical_device, render_pass, quad_shader.get(), m_quad_material.get(), 1);

        // Create quad model
        std::vector<float>    vertices = {-1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
                                          1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f};
        std::vector<uint32_t> indices  = {0, 1, 2, 0, 2, 3};

        m_quad_model =
            std::move(Model(std::move(vertices), std::move(indices), m_quad_material->shader->per_vertex_attributes));

        input_vertex_attributes = m_obj2attachment_material->shader->per_vertex_attributes;

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

        auto skybox_shader = shader_factory.clear()
                                 .SetVertexShader("builtin/shaders/skybox.vert.spv")
                                 .SetFragmentShader("builtin/shaders/skybox.frag.spv")
                                 .Create();

        m_skybox_material = std::make_shared<Material>(skybox_shader);
        g_runtime_context.resource_system->Register(m_skybox_material);
        material_factory.Init(skybox_shader.get(), vk::FrontFace::eCounterClockwise);
        material_factory.SetOpaque(true, 1);
        material_factory.CreatePipeline(logical_device, render_pass, skybox_shader.get(), m_skybox_material.get(), 2);

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
                m_skybox_material->BindImageToDescriptorSet("environmentMap", *texture_ptr);
            }

            texture_ptr->SetDebugName("Skybox Texture");
        }

        GeometryFactory geometry_factory;
        geometry_factory.SetCube();
        auto cube_vertices = geometry_factory.GetVertices(skybox_shader->per_vertex_attributes);
        m_skybox_model =
            std::move(Model(cube_vertices, std::vector<uint32_t> {}, skybox_shader->per_vertex_attributes));
    }

    void DeferredPassBase::RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
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

        m_color_attachment->SetDebugName("Deferred Color Attachment");
        m_normal_attachment->SetDebugName("Deferred Normal Attachment");
        m_position_attachment->SetDebugName("Deferred Position Attachment");
        m_depth_attachment->SetDebugName("Deferred Depth Attachment");

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

        m_quad_material->BindImageToDescriptorSet("inputColor", *m_color_attachment);
        m_quad_material->BindImageToDescriptorSet("inputNormal", *m_normal_attachment);
        m_quad_material->BindImageToDescriptorSet("inputPosition", *m_position_attachment);
        m_quad_material->BindImageToDescriptorSet("inputDepth", *m_depth_attachment);
    }

    void DeferredPassBase::UpdateUniformBuffer(uint32_t frame_index)
    {
        FUNCTION_TIMER();

        std::shared_ptr<Level> level = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

        if (!level)
            MEOW_ERROR("shared ptr is invalid!");

        std::shared_ptr<GameObject>           main_camera = level->GetGameObjectByID(level->GetMainCameraID()).lock();
        std::shared_ptr<Transform3DComponent> main_camera_transfrom_component =
            main_camera->TryGetComponent<Transform3DComponent>("Transform3DComponent");
        std::shared_ptr<Camera3DComponent> main_camera_component =
            main_camera->TryGetComponent<Camera3DComponent>("Camera3DComponent");

        if (!main_camera)
            MEOW_ERROR("shared ptr is invalid!");
        if (!main_camera_transfrom_component)
            MEOW_ERROR("shared ptr is invalid!");
        if (!main_camera_component)
            MEOW_ERROR("shared ptr is invalid!");

        glm::ivec2 window_size = g_runtime_context.window_system->GetCurrentFocusWindow()->GetSize();

        glm::vec3 forward = main_camera_transfrom_component->rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        glm::mat4 view    = lookAt(main_camera_transfrom_component->position,
                                main_camera_transfrom_component->position + forward,
                                glm::vec3(0.0f, 1.0f, 0.0f));

        PerSceneData per_scene_data;
        per_scene_data.view = view;
        per_scene_data.projection =
            Math::perspective_vk(main_camera_component->field_of_view,
                                 static_cast<float>(window_size[0]) / static_cast<float>(window_size[1]),
                                 main_camera_component->near_plane,
                                 main_camera_component->far_plane);

        m_obj2attachment_material->PopulateUniformBuffer(
            "sceneData", &per_scene_data, sizeof(per_scene_data), frame_index);

        // Update mesh uniform

        m_obj2attachment_material->BeginPopulatingDynamicUniformBufferPerFrame();
        const auto* visibles_opaque_ptr = level->GetVisiblesPerShadingModel(ShadingModelType::Opaque);
        if (visibles_opaque_ptr)
        {
            const auto& visibles_opaque = *visibles_opaque_ptr;
            for (const auto& visible : visibles_opaque)
            {
                std::shared_ptr<GameObject>           current_gameobject = visible.lock();
                std::shared_ptr<Transform3DComponent> current_gameobject_transfrom_component =
                    current_gameobject->TryGetComponent<Transform3DComponent>("Transform3DComponent");
                std::shared_ptr<ModelComponent> current_gameobject_model_component =
                    current_gameobject->TryGetComponent<ModelComponent>("ModelComponent");

                if (!current_gameobject_transfrom_component || !current_gameobject_model_component)
                    continue;

#ifdef MEOW_DEBUG
                if (!current_gameobject)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!current_gameobject_transfrom_component)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!current_gameobject_model_component)
                    MEOW_ERROR("shared ptr is invalid!");
#endif

                auto model = current_gameobject_transfrom_component->GetTransform();

                for (uint32_t i = 0; i < current_gameobject_model_component->model.lock()->meshes.size(); ++i)
                {
                    m_obj2attachment_material->BeginPopulatingDynamicUniformBufferPerObject();
                    m_obj2attachment_material->PopulateDynamicUniformBuffer(
                        "objData", &model, sizeof(model), frame_index);
                    m_obj2attachment_material->EndPopulatingDynamicUniformBufferPerObject();
                }
            }
            m_obj2attachment_material->EndPopulatingDynamicUniformBufferPerFrame();
        }

        // update light

        for (int32_t i = 0; i < k_num_lights; ++i)
        {
            float bias = glm::sin(g_runtime_context.time_system->GetTime() * m_LightInfos.speed[i]) / 5.0f;
            m_LightDatas.lights[i].position.x = m_LightInfos.position[i].x + bias * m_LightInfos.direction[i].x;
            m_LightDatas.lights[i].position.y = m_LightInfos.position[i].y + bias * m_LightInfos.direction[i].y;
            m_LightDatas.lights[i].position.z = m_LightInfos.position[i].z + bias * m_LightInfos.direction[i].z;
        }

        m_quad_material->PopulateUniformBuffer("lightDatas", &m_LightDatas, sizeof(m_LightDatas), frame_index);

        // skybox

        per_scene_data.view = lookAt(glm::vec3(0.0f), glm::vec3(0.0f) + forward, glm::vec3(0.0f, 1.0f, 0.0f));
        m_skybox_material->PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data), frame_index);
    }

    void
    DeferredPassBase::Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index)
    {
        for (int i = 0; i < 2; i++)
        {
            draw_call[i] = 0;
        }

        RenderPassBase::Start(command_buffer, extent, image_index);

        command_buffer.setViewport(0,
                                   vk::Viewport(0.0f,
                                                static_cast<float>(extent.height),
                                                static_cast<float>(extent.width),
                                                -static_cast<float>(extent.height),
                                                0.0f,
                                                1.0f));
        command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
    }

    void DeferredPassBase::RenderGBuffer(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_obj2attachment_material->BindDescriptorSetToPipeline(command_buffer, 0, 1);

        std::shared_ptr<Level> level               = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        const auto*            visibles_opaque_ptr = level->GetVisiblesPerShadingModel(ShadingModelType::Opaque);
        if (visibles_opaque_ptr)
        {
            const auto& visibles_opaque = *visibles_opaque_ptr;
            for (const auto& visible : visibles_opaque)
            {
                std::shared_ptr<GameObject> current_gameobject = visible.lock();
                if (!current_gameobject)
                    continue;

                std::shared_ptr<ModelComponent> current_gameobject_model_component =
                    current_gameobject->TryGetComponent<ModelComponent>("ModelComponent");
                if (!current_gameobject_model_component)
                    continue;

                auto model_resource = current_gameobject_model_component->model.lock();
                if (!model_resource)
                    continue;

                for (uint32_t i = 0; i < model_resource->meshes.size(); ++i)
                {
                    m_obj2attachment_material->BindDescriptorSetToPipeline(command_buffer, 1, 1, draw_call[0], true);
                    model_resource->meshes[i]->BindDrawCmd(command_buffer);

                    ++draw_call[0];
                }
            }
        }
    }

    void DeferredPassBase::RenderOpaqueMeshes(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_quad_material->BindDescriptorSetToPipeline(command_buffer, 0, 1);
        for (int32_t i = 0; i < m_quad_model.meshes.size(); ++i)
        {
            m_quad_model.meshes[i]->BindDrawCmd(command_buffer);

            ++draw_call[1];
        }
    }

    void DeferredPassBase::RenderSkybox(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_skybox_material->BindDescriptorSetToPipeline(command_buffer, 0, 2);

        m_skybox_model.meshes[0]->BindDrawCmd(command_buffer);
    }

    void swap(DeferredPassBase& lhs, DeferredPassBase& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPassBase&>(lhs), static_cast<RenderPassBase&>(rhs));

        swap(lhs.m_obj2attachment_material, rhs.m_obj2attachment_material);
        swap(lhs.m_quad_material, rhs.m_quad_material);
        swap(lhs.m_quad_model, rhs.m_quad_model);
        swap(lhs.m_skybox_material, rhs.m_skybox_material);
        swap(lhs.m_skybox_model, rhs.m_skybox_model);

        swap(lhs.m_color_attachment, rhs.m_color_attachment);
        swap(lhs.m_normal_attachment, rhs.m_normal_attachment);
        swap(lhs.m_position_attachment, rhs.m_position_attachment);

        swap(lhs.m_LightDatas, rhs.m_LightDatas);
        swap(lhs.m_LightInfos, rhs.m_LightInfos);

        swap(lhs.m_depth_attachment, rhs.m_depth_attachment);

        swap(lhs.m_pass_names, rhs.m_pass_names);
        swap(lhs.draw_call, rhs.draw_call);
    }
} // namespace Meow