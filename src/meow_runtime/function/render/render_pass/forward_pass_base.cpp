#include "forward_pass_base.h"

#include "pch.h"

#include "core/math/math.h"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/light/directional_light_component.h"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_context.h"
#include "function/render/buffer_data/per_scene_data.h"
#include "function/render/geometry/geometry_factory.h"
#include "function/render/material/material_factory.h"
#include "function/render/material/shader_factory.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    void ForwardPassBase::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        ShaderFactory   shader_factory;
        MaterialFactory material_factory;

        auto opaque_shader = shader_factory.clear()
                                 .SetVertexShader("builtin/shaders/pbr.vert.spv")
                                 .SetFragmentShader("builtin/shaders/pbr.frag.spv")
                                 .Create();

        m_opaque_material = std::make_shared<Material>(opaque_shader);
        g_runtime_context.resource_system->Register(m_opaque_material);
        material_factory.Init(opaque_shader.get(), vk::FrontFace::eClockwise);
        material_factory.SetMSAA(m_msaa_enabled);
        material_factory.SetOpaque(true, 1);
        material_factory.CreatePipeline(logical_device, render_pass, opaque_shader.get(), m_opaque_material.get(), 0);
        m_opaque_material->SetDebugName("Forward Opaque Material");

        input_vertex_attributes = m_opaque_material->shader->per_vertex_attributes;

        UUID albedo_image_id(0);
        UUID normal_image_id(0);
        UUID metallic_image_id(0);
        UUID roughness_image_id(0);
        UUID ao_image_id(0);
        UUID irradiance_image_id(0);

        {
            auto texture_ptr = ImageData::CreateTexture("builtin/textures/pbr_sphere/albedo.png");
            if (texture_ptr)
            {
                albedo_image_id = g_runtime_context.resource_system->Register(texture_ptr);
                m_opaque_material->BindImageToDescriptorSet("albedoMap", *texture_ptr);
            }

            texture_ptr->SetDebugName("Albedo Texture");
        }

        {
            auto texture_ptr = ImageData::CreateTexture("builtin/textures/pbr_sphere/normal.png");
            if (texture_ptr)
            {
                normal_image_id = g_runtime_context.resource_system->Register(texture_ptr);
                m_opaque_material->BindImageToDescriptorSet("normalMap", *texture_ptr);
            }

            texture_ptr->SetDebugName("Normal Texture");
        }

        {
            auto texture_ptr = ImageData::CreateTexture("builtin/textures/pbr_sphere/metallic.png");
            if (texture_ptr)
            {
                metallic_image_id = g_runtime_context.resource_system->Register(texture_ptr);
                m_opaque_material->BindImageToDescriptorSet("metallicMap", *texture_ptr);
            }

            texture_ptr->SetDebugName("Metallic Texture");
        }

        {
            auto texture_ptr = ImageData::CreateTexture("builtin/textures/pbr_sphere/roughness.png");
            if (texture_ptr)
            {
                roughness_image_id = g_runtime_context.resource_system->Register(texture_ptr);
                m_opaque_material->BindImageToDescriptorSet("roughnessMap", *texture_ptr);
            }

            texture_ptr->SetDebugName("Roughness Texture");
        }

        {
            auto texture_ptr = ImageData::CreateTexture("builtin/textures/pbr_sphere/ao.png");
            if (texture_ptr)
            {
                ao_image_id = g_runtime_context.resource_system->Register(texture_ptr);
                m_opaque_material->BindImageToDescriptorSet("aoMap", *texture_ptr);
            }

            texture_ptr->SetDebugName("AO Texture");
        }

        {
            auto texture_ptr = ImageData::CreateCubemap({
                "builtin/textures/cubemap/skybox_irradiance_X+.hdr",
                "builtin/textures/cubemap/skybox_irradiance_X-.hdr",
                "builtin/textures/cubemap/skybox_irradiance_Z+.hdr",
                "builtin/textures/cubemap/skybox_irradiance_Z-.hdr",
                "builtin/textures/cubemap/skybox_irradiance_Y+.hdr",
                "builtin/textures/cubemap/skybox_irradiance_Y-.hdr",
            });
            if (texture_ptr)
            {
                irradiance_image_id = g_runtime_context.resource_system->Register(texture_ptr);
                m_opaque_material->BindImageToDescriptorSet("irradianceMap", *texture_ptr);
            }

            texture_ptr->SetDebugName("Irradiance Texture");
        }

        // skybox

        auto skybox_shader = shader_factory.clear()
                                 .SetVertexShader("builtin/shaders/skybox.vert.spv")
                                 .SetFragmentShader("builtin/shaders/skybox.frag.spv")
                                 .Create();

        m_skybox_material = std::make_shared<Material>(skybox_shader);
        g_runtime_context.resource_system->Register(m_skybox_material);
        material_factory.Init(skybox_shader.get(), vk::FrontFace::eCounterClockwise);
        material_factory.SetMSAA(m_msaa_enabled);
        material_factory.SetOpaque(true, 1);
        material_factory.CreatePipeline(logical_device, render_pass, skybox_shader.get(), m_skybox_material.get(), 0);
        m_skybox_material->SetDebugName("Forward Skybox Material");

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

        auto translucent_shader = shader_factory.clear()
                                      .SetVertexShader("builtin/shaders/pbr_translucent.vert.spv")
                                      .SetFragmentShader("builtin/shaders/pbr_translucent.frag.spv")
                                      .Create();

        m_translucent_material = std::make_shared<Material>(translucent_shader);
        g_runtime_context.resource_system->Register(m_translucent_material);
        material_factory.Init(translucent_shader.get(), vk::FrontFace::eClockwise);
        material_factory.SetMSAA(m_msaa_enabled);
        material_factory.SetTranslucent(true, 1);
        material_factory.CreatePipeline(
            logical_device, render_pass, translucent_shader.get(), m_translucent_material.get(), 0);
        m_translucent_material->SetDebugName("Forward Translucent Material");

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(albedo_image_id);
            if (texture_ptr)
            {
                m_translucent_material->BindImageToDescriptorSet("albedoMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(normal_image_id);
            if (texture_ptr)
            {
                m_translucent_material->BindImageToDescriptorSet("normalMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(metallic_image_id);
            if (texture_ptr)
            {
                m_translucent_material->BindImageToDescriptorSet("metallicMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(roughness_image_id);
            if (texture_ptr)
            {
                m_translucent_material->BindImageToDescriptorSet("roughnessMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(ao_image_id);
            if (texture_ptr)
            {
                m_translucent_material->BindImageToDescriptorSet("aoMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(irradiance_image_id);
            if (texture_ptr)
            {
                m_translucent_material->BindImageToDescriptorSet("irradianceMap", *texture_ptr);
            }
        }
    }

    void ForwardPassBase::PopulateDirectionalLightData(std::shared_ptr<ImageData> shadow_map)
    {
        std::shared_ptr<Level> level = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

        const auto& all_gameobjects_map = level->GetAllGameObjects();
        for (auto& kv : all_gameobjects_map)
        {
            std::shared_ptr<DirectionalLightComponent> directional_light_comp_ptr =
                kv.second->TryGetComponent<DirectionalLightComponent>("DirectionalLightComponent");
            if (directional_light_comp_ptr)
            {
                std::shared_ptr<Transform3DComponent> directional_light_transform =
                    kv.second->TryGetComponent<Transform3DComponent>("Transform3DComponent");

                if (!directional_light_transform)
                {
                    continue;
                }

                glm::vec3 forward = directional_light_transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f);

                // TODO: PerLightData
                PerSceneData per_light_data;
                per_light_data.view = lookAt(directional_light_transform->position,
                                             directional_light_transform->position + forward,
                                             glm::vec3(0.0f, 1.0f, 0.0f));
                per_light_data.projection =
                    Math::perspective_vk(directional_light_comp_ptr->field_of_view,
                                         (float)shadow_map->extent.width / shadow_map->extent.height,
                                         directional_light_comp_ptr->near_plane,
                                         directional_light_comp_ptr->far_plane);
                m_opaque_material->PopulateUniformBuffer("lightData", &per_light_data, sizeof(per_light_data));
                break;
            }
        }
    }

    void ForwardPassBase::BindShadowMap(std::shared_ptr<ImageData> shadow_map)
    {
        m_opaque_material->BindImageToDescriptorSet("shadowMap", *shadow_map);
    }

    void ForwardPassBase::RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                              const vk::Extent2D&               extent)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // clear

        framebuffers.clear();

        m_depth_attachment = nullptr;

        // Create attachment

        vk::SampleCountFlagBits sample_count =
            m_msaa_enabled ? g_runtime_context.render_system->GetMSAASamples() : vk::SampleCountFlagBits::e1;
        m_msaa_enabled = (sample_count != vk::SampleCountFlagBits::e1);

        vk::ImageUsageFlags depth_msaa_usage    = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        vk::ImageUsageFlags depth_resolve_usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;

        // if (g_runtime_context.render_system->GetPostProcessRunning())
        // {
        //     // Depth needs to be read by the postprocessing subpass
        //     if (msaa_enabled && g_runtime_context.render_system->GetDepthWritebackResolveSupported() &&
        //         g_runtime_context.render_system->GetResolveDepthOnWriteback())
        //     {
        //         // Depth is resolved
        //         depth_msaa_usage |= vk::ImageUsageFlagBits::eTransientAttachment;
        //         depth_resolve_usage |= vk::ImageUsageFlagBits::eSampled;
        //     }
        //     else
        //     {
        //         // Postprocessing reads multisampled depth
        //         depth_msaa_usage |= vk::ImageUsageFlagBits::eSampled;
        //         depth_resolve_usage |= vk::ImageUsageFlagBits::eTransientAttachment;
        //     }
        // }
        // else
        // {
        //     // Depth attachments are transient
        //     depth_msaa_usage |= vk::ImageUsageFlagBits::eTransientAttachment;
        //     depth_resolve_usage |= vk::ImageUsageFlagBits::eTransientAttachment;
        // }

        m_depth_attachment = ImageData::CreateAttachment(
            m_depth_format, extent, depth_msaa_usage, vk::ImageAspectFlagBits::eDepth, {}, false, sample_count);

        m_depth_attachment->SetDebugName("Depth Attachment");

        m_color_msaa_attachment = nullptr;
        if (m_msaa_enabled)
        {
            vk::ImageUsageFlags color_msaa_usage =
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment;
            m_color_msaa_attachment = ImageData::CreateAttachment(
                m_color_format, extent, color_msaa_usage, vk::ImageAspectFlagBits::eColor, {}, false, sample_count);

            m_color_msaa_attachment->SetDebugName("Color MSAA Attachment");
        }

        // Provide attachment information to frame buffer

        if (m_msaa_enabled)
        {
            vk::ImageView attachments[3];
            attachments[0] = *m_color_msaa_attachment->image_view;
            attachments[1] = *m_depth_attachment->image_view;

            vk::FramebufferCreateInfo framebuffer_create_info(vk::FramebufferCreateFlags(), /* flags */
                                                              *render_pass,                 /* renderPass */
                                                              3,                            /* attachmentCount */
                                                              attachments,                  /* pAttachments */
                                                              extent.width,                 /* width */
                                                              extent.height,                /* height */
                                                              1);                           /* layers */

            framebuffers.reserve(output_image_views.size());
            for (const auto& imageView : output_image_views)
            {
                attachments[2] = imageView;
                framebuffers.push_back(vk::raii::Framebuffer(logical_device, framebuffer_create_info));
            }
        }
        else
        {
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
                framebuffers.push_back(vk::raii::Framebuffer(logical_device, framebuffer_create_info));
            }
        }
    }

    void ForwardPassBase::UpdateUniformBuffer()
    {
        FUNCTION_TIMER();

        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

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

        const auto* visibles_opaque_ptr = level->GetVisiblesPerShadingModel(ShadingModelType::Opaque);

        // Opaque

        PerSceneData per_scene_data;
        per_scene_data.view = view;
        per_scene_data.projection =
            Math::perspective_vk(main_camera_component->field_of_view,
                                 static_cast<float>(window_size[0]) / static_cast<float>(window_size[1]),
                                 main_camera_component->near_plane,
                                 main_camera_component->far_plane);

        m_opaque_material->PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));

        struct LightData
        {
            glm::vec4 pos[4] = {
                glm::vec4(-10.0f, 10.0f, 10.0f, 0.0f),
                glm::vec4(10.0f, 10.0f, 10.0f, 0.0f),
                glm::vec4(-10.0f, -10.0f, 10.0f, 0.0f),
                glm::vec4(10.0f, -10.0f, 10.0f, 0.0f),
            };
            glm::vec4 color[4] = {
                glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
                glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
                glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
                glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
            };
            glm::vec3 camPos;
        };

        LightData lights;
        lights.camPos = main_camera_transfrom_component->position;

        m_opaque_material->PopulateUniformBuffer("lights", &lights, sizeof(lights));

        // Update mesh uniform

        m_opaque_material->BeginPopulatingDynamicUniformBufferPerFrame();
        if (visibles_opaque_ptr)
        {
            const auto& visibles_opaque = *visibles_opaque_ptr;
            for (const auto& visible : visibles_opaque)
            {
                std::shared_ptr<GameObject> current_gameobject = visible.lock();
                if (!current_gameobject)
                    continue;
                std::shared_ptr<Transform3DComponent> current_gameobject_transfrom_component =
                    current_gameobject->TryGetComponent<Transform3DComponent>("Transform3DComponent");
                std::shared_ptr<ModelComponent> current_gameobject_model_component =
                    current_gameobject->TryGetComponent<ModelComponent>("ModelComponent");

                if (!current_gameobject_transfrom_component || !current_gameobject_model_component)
                    continue;

                if (!current_gameobject)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!current_gameobject_transfrom_component)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!current_gameobject_model_component)
                    MEOW_ERROR("shared ptr is invalid!");

                auto model = current_gameobject_transfrom_component->GetTransform();

                for (uint32_t i = 0; i < current_gameobject_model_component->model.lock()->meshes.size(); ++i)
                {
                    m_opaque_material->BeginPopulatingDynamicUniformBufferPerObject();
                    m_opaque_material->PopulateDynamicUniformBuffer("objData", &model, sizeof(model));
                    m_opaque_material->EndPopulatingDynamicUniformBufferPerObject();
                }
            }
            m_opaque_material->EndPopulatingDynamicUniformBufferPerFrame();
        }

        // skybox

        per_scene_data.view = lookAt(glm::vec3(0.0f), glm::vec3(0.0f) + forward, glm::vec3(0.0f, 1.0f, 0.0f));
        m_skybox_material->PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));

        // translucent

        struct TranslucentObjectData
        {
            glm::mat4 model;
            float     alpha;
        };

        per_scene_data.view = view;
        m_translucent_material->PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));
        m_translucent_material->PopulateUniformBuffer("lights", &lights, sizeof(lights));
        m_translucent_material->BeginPopulatingDynamicUniformBufferPerFrame();
        const auto* visibles_translucent_ptr = level->GetVisiblesPerShadingModel(ShadingModelType::Translucent);
        if (visibles_translucent_ptr)
        {
            const auto& visibles_translucent = *visibles_translucent_ptr;

            int visibles_size = visibles_translucent.size();

            for (int obj_index = 0; obj_index < visibles_size; obj_index++)
            {
                const auto&                 visible            = visibles_translucent[obj_index];
                std::shared_ptr<GameObject> current_gameobject = visible.lock();
                if (!current_gameobject)
                    continue;
                std::shared_ptr<Transform3DComponent> current_gameobject_transfrom_component =
                    current_gameobject->TryGetComponent<Transform3DComponent>("Transform3DComponent");
                std::shared_ptr<ModelComponent> current_gameobject_model_component =
                    current_gameobject->TryGetComponent<ModelComponent>("ModelComponent");

                if (!current_gameobject_transfrom_component || !current_gameobject_model_component)
                    continue;

                if (!current_gameobject)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!current_gameobject_transfrom_component)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!current_gameobject_model_component)
                    MEOW_ERROR("shared ptr is invalid!");

                TranslucentObjectData translucent_obj_data;

                translucent_obj_data.model = current_gameobject_transfrom_component->GetTransform();
                translucent_obj_data.alpha = (float)obj_index / visibles_size;
                for (uint32_t i = 0; i < current_gameobject_model_component->model.lock()->meshes.size(); ++i)
                {
                    m_translucent_material->BeginPopulatingDynamicUniformBufferPerObject();
                    m_translucent_material->PopulateDynamicUniformBuffer(
                        "objData", &translucent_obj_data, sizeof(translucent_obj_data));
                    m_translucent_material->EndPopulatingDynamicUniformBufferPerObject();
                }
            }
            m_translucent_material->EndPopulatingDynamicUniformBufferPerFrame();
        }
    }

    void
    ForwardPassBase::Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index)
    {
        for (int i = 0; i < 3; i++)
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

    void ForwardPassBase::RenderOpaqueMeshes(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_opaque_material->BindDescriptorSetToPipeline(command_buffer, 0, 2);
        m_opaque_material->BindDescriptorSetToPipeline(command_buffer, 3, 1);

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
                    m_opaque_material->BindDescriptorSetToPipeline(command_buffer, 2, 1, draw_call[0], true);
                    model_resource->meshes[i]->BindDrawCmd(command_buffer);

                    ++draw_call[0];
                }
            }
        }
    }

    void ForwardPassBase::RenderSkybox(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_skybox_material->BindDescriptorSetToPipeline(command_buffer, 0, 2);

        m_skybox_model.meshes[0]->BindDrawCmd(command_buffer);
        ++draw_call[1];
    }

    void ForwardPassBase::RenderTranslucentMeshes(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_translucent_material->BindDescriptorSetToPipeline(command_buffer, 0, 2);
        m_translucent_material->BindDescriptorSetToPipeline(command_buffer, 3, 1);

        std::shared_ptr<Level> level = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

        std::shared_ptr<GameObject>           main_camera = level->GetGameObjectByID(level->GetMainCameraID()).lock();
        std::shared_ptr<Transform3DComponent> camera_transform_ptr =
            main_camera->TryGetComponent<Transform3DComponent>("Transform3DComponent");
        if (!camera_transform_ptr)
            return;

        glm::vec3 camera_pos     = camera_transform_ptr->position;
        glm::vec3 camera_forward = camera_transform_ptr->rotation * glm::vec3(0.0f, 0.0f, 1.0f);

        auto* visibles_translucent_ptr = level->GetVisiblesPerShadingModel(ShadingModelType::Translucent);
        if (visibles_translucent_ptr)
        {
            auto& visibles_translucent = *visibles_translucent_ptr; // std::vector<std::weak_ptr<Meow::GameObject>>

            std::sort(visibles_translucent.begin(),
                      visibles_translucent.end(),
                      [&](const std::weak_ptr<GameObject>& a, const std::weak_ptr<GameObject>& b) {
                          auto a_ptr = a.lock();
                          auto b_ptr = b.lock();
                          if (!a_ptr || !b_ptr)
                              return false;

                          auto a_transform = a_ptr->TryGetComponent<Transform3DComponent>("Transform3DComponent");
                          auto b_transform = b_ptr->TryGetComponent<Transform3DComponent>("Transform3DComponent");
                          if (!a_transform || !b_transform)
                              return false;

                          float dist_a = glm::dot(a_transform->position - camera_pos, camera_forward);
                          float dist_b = glm::dot(b_transform->position - camera_pos, camera_forward);

                          return dist_a > dist_b;
                      });

            for (const auto& visible : visibles_translucent)
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
                    m_translucent_material->BindDescriptorSetToPipeline(command_buffer, 2, 1, draw_call[2], true);
                    model_resource->meshes[i]->BindDrawCmd(command_buffer);

                    ++draw_call[2];
                }
            }
        }
    }

    void ForwardPassBase::SetMSAAEnabled(bool enabled)
    {
        if (m_msaa_enabled == enabled)
        {
            return;
        }

        m_msaa_enabled = enabled;
    }

    void swap(ForwardPassBase& lhs, ForwardPassBase& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPassBase&>(lhs), static_cast<RenderPassBase&>(rhs));

        swap(lhs.m_opaque_material, rhs.m_opaque_material);
        swap(lhs.m_skybox_material, rhs.m_skybox_material);
        swap(lhs.m_skybox_model, rhs.m_skybox_model);
        swap(lhs.m_translucent_material, rhs.m_translucent_material);

        swap(lhs.m_depth_attachment, rhs.m_depth_attachment);

        swap(lhs.m_pass_names, rhs.m_pass_names);
        swap(lhs.draw_call, rhs.draw_call);
    }
} // namespace Meow
