#include "forward_pass.h"

#include "pch.h"

#include "core/math/math.h"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_context.h"
#include "function/render/buffer_data/per_scene_data.h"
#include "function/render/material/material_factory.h"
#include "function/render/utils/model_utils.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    void ForwardPass::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        MaterialFactory material_factory;

        auto mesh_shader_ptr = std::make_shared<Shader>(
            physical_device, logical_device, "builtin/shaders/pbr.vert.spv", "builtin/shaders/pbr.frag.spv");

        m_forward_mat = Material(mesh_shader_ptr);
        material_factory.Init(mesh_shader_ptr.get(), vk::FrontFace::eClockwise);
        material_factory.SetOpaque(true, 1);
        material_factory.CreatePipeline(logical_device, render_pass, mesh_shader_ptr.get(), &m_forward_mat, 0);

        input_vertex_attributes = m_forward_mat.shader_ptr->per_vertex_attributes;

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
                m_forward_mat.BindImageToDescriptorSet("albedoMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = ImageData::CreateTexture("builtin/textures/pbr_sphere/normal.png");
            if (texture_ptr)
            {
                normal_image_id = g_runtime_context.resource_system->Register(texture_ptr);
                m_forward_mat.BindImageToDescriptorSet("normalMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = ImageData::CreateTexture("builtin/textures/pbr_sphere/metallic.png");
            if (texture_ptr)
            {
                metallic_image_id = g_runtime_context.resource_system->Register(texture_ptr);
                m_forward_mat.BindImageToDescriptorSet("metallicMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = ImageData::CreateTexture("builtin/textures/pbr_sphere/roughness.png");
            if (texture_ptr)
            {
                roughness_image_id = g_runtime_context.resource_system->Register(texture_ptr);
                m_forward_mat.BindImageToDescriptorSet("roughnessMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = ImageData::CreateTexture("builtin/textures/pbr_sphere/ao.png");
            if (texture_ptr)
            {
                ao_image_id = g_runtime_context.resource_system->Register(texture_ptr);
                m_forward_mat.BindImageToDescriptorSet("aoMap", *texture_ptr);
            }
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
                m_forward_mat.BindImageToDescriptorSet("irradianceMap", *texture_ptr);
            }
        }

        // skybox

        auto skybox_shader_ptr = std::make_shared<Shader>(
            physical_device, logical_device, "builtin/shaders/skybox.vert.spv", "builtin/shaders/skybox.frag.spv");

        m_skybox_mat = Material(skybox_shader_ptr);
        material_factory.Init(skybox_shader_ptr.get(), vk::FrontFace::eCounterClockwise);
        material_factory.SetOpaque(true, 1);
        material_factory.CreatePipeline(logical_device, render_pass, skybox_shader_ptr.get(), &m_skybox_mat, 0);

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

        auto translucent_shader_ptr = std::make_shared<Shader>(physical_device,
                                                               logical_device,
                                                               "builtin/shaders/pbr_translucent.vert.spv",
                                                               "builtin/shaders/pbr_translucent.frag.spv");

        m_translucent_mat = Material(translucent_shader_ptr);
        material_factory.Init(translucent_shader_ptr.get(), vk::FrontFace::eClockwise);
        material_factory.SetTranslucent(true, 1);
        material_factory.CreatePipeline(
            logical_device, render_pass, translucent_shader_ptr.get(), &m_translucent_mat, 0);

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(albedo_image_id);
            if (texture_ptr)
            {
                m_forward_mat.BindImageToDescriptorSet("albedoMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(normal_image_id);
            if (texture_ptr)
            {
                m_forward_mat.BindImageToDescriptorSet("normalMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(metallic_image_id);
            if (texture_ptr)
            {
                m_forward_mat.BindImageToDescriptorSet("metallicMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(roughness_image_id);
            if (texture_ptr)
            {
                m_forward_mat.BindImageToDescriptorSet("roughnessMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(ao_image_id);
            if (texture_ptr)
            {
                m_forward_mat.BindImageToDescriptorSet("aoMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr = g_runtime_context.resource_system->GetResource<ImageData>(irradiance_image_id);
            if (texture_ptr)
            {
                m_forward_mat.BindImageToDescriptorSet("irradianceMap", *texture_ptr);
            }
        }
    }

    void ForwardPass::RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                          const vk::Extent2D&               extent)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // clear

        framebuffers.clear();

        m_depth_attachment = nullptr;

        // Create attachment

        m_depth_attachment = ImageData::CreateAttachment(m_depth_format,
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
            framebuffers.push_back(vk::raii::Framebuffer(logical_device, framebuffer_create_info));
        }
    }

    void ForwardPass::UpdateUniformBuffer()
    {
        FUNCTION_TIMER();

        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

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

        m_forward_mat.PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));

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
        lights.camPos = transfrom_comp_ptr->position;

        m_forward_mat.PopulateUniformBuffer("lights", &lights, sizeof(lights));

        // Update mesh uniform

        m_forward_mat.BeginPopulatingDynamicUniformBufferPerFrame();
        const auto* visibles_forward_ptr = level_ptr->GetVisiblesPerMaterial(m_forward_mat.uuid);
        if (visibles_forward_ptr)
        {
            const auto& visibles_forward = *visibles_forward_ptr;
            for (const auto& visible : visibles_forward)
            {
                std::shared_ptr<GameObject> gameobject_ptr = visible.lock();
                if (!gameobject_ptr)
                    continue;
                std::shared_ptr<Transform3DComponent> transfrom_comp_ptr2 =
                    gameobject_ptr->TryGetComponent<Transform3DComponent>("Transform3DComponent");
                std::shared_ptr<ModelComponent> model_ptr =
                    gameobject_ptr->TryGetComponent<ModelComponent>("ModelComponent");

                if (!transfrom_comp_ptr2 || !model_ptr)
                    continue;

#ifdef MEOW_DEBUG
                if (!gameobject_ptr)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!transfrom_comp_ptr2)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!model_ptr)
                    MEOW_ERROR("shared ptr is invalid!");
#endif

                auto model = transfrom_comp_ptr2->GetTransform();

                for (uint32_t i = 0; i < model_ptr->model_ptr.lock()->meshes.size(); ++i)
                {
                    m_forward_mat.BeginPopulatingDynamicUniformBufferPerObject();
                    m_forward_mat.PopulateDynamicUniformBuffer("objData", &model, sizeof(model));
                    m_forward_mat.EndPopulatingDynamicUniformBufferPerObject();
                }
            }
            m_forward_mat.EndPopulatingDynamicUniformBufferPerFrame();
        }

        // skybox

        per_scene_data.view = lookAt(glm::vec3(0.0f), glm::vec3(0.0f) + forward, glm::vec3(0.0f, 1.0f, 0.0f));
        m_skybox_mat.PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));

        // translucent

        struct TranslucentObjectData
        {
            glm::mat4 model;
            float     alpha;
            float     padding1;
            float     padding2;
            float     padding3;
        };

        m_translucent_mat.PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));
        m_translucent_mat.PopulateUniformBuffer("lights", &lights, sizeof(lights));
        m_translucent_mat.BeginPopulatingDynamicUniformBufferPerFrame();
        const auto* visibles_translucent_ptr = level_ptr->GetVisiblesPerMaterial(m_translucent_mat.uuid);
        if (visibles_translucent_ptr)
        {
            const auto& visibles_translucent = *visibles_translucent_ptr;

            int visibles_size = visibles_translucent.size();

            for (int obj_index = 0; obj_index < visibles_size; obj_index++)
            {
                const auto&                 visible        = visibles_translucent[obj_index];
                std::shared_ptr<GameObject> gameobject_ptr = visible.lock();
                if (!gameobject_ptr)
                    continue;
                std::shared_ptr<Transform3DComponent> transfrom_comp_ptr2 =
                    gameobject_ptr->TryGetComponent<Transform3DComponent>("Transform3DComponent");
                std::shared_ptr<ModelComponent> model_ptr =
                    gameobject_ptr->TryGetComponent<ModelComponent>("ModelComponent");

                if (!transfrom_comp_ptr2 || !model_ptr)
                    continue;

#ifdef MEOW_DEBUG
                if (!gameobject_ptr)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!transfrom_comp_ptr2)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!model_ptr)
                    MEOW_ERROR("shared ptr is invalid!");
#endif

                TranslucentObjectData translucent_obj_data;

                translucent_obj_data.model = transfrom_comp_ptr2->GetTransform();
                translucent_obj_data.alpha = (float)obj_index / visibles_size;
                for (uint32_t i = 0; i < model_ptr->model_ptr.lock()->meshes.size(); ++i)
                {
                    m_translucent_mat.BeginPopulatingDynamicUniformBufferPerObject();
                    m_translucent_mat.PopulateDynamicUniformBuffer(
                        "objData", &translucent_obj_data, sizeof(translucent_obj_data));
                    m_translucent_mat.EndPopulatingDynamicUniformBufferPerObject();
                }
            }
            m_translucent_mat.EndPopulatingDynamicUniformBufferPerFrame();
        }
    }

    void
    ForwardPass::Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t current_image_index)
    {
        for (int i = 0; i < 2; i++)
        {
            draw_call[i] = 0;
        }

        RenderPass::Start(command_buffer, extent, current_image_index);
    }

    void ForwardPass::MeshLighting(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_forward_mat.BindDescriptorSetToPipeline(command_buffer, 0, 1);
        m_forward_mat.BindDescriptorSetToPipeline(command_buffer, 3, 1);

        std::shared_ptr<Level> level_ptr            = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        const auto*            visibles_forward_ptr = level_ptr->GetVisiblesPerMaterial(m_forward_mat.uuid);
        if (visibles_forward_ptr)
        {
            const auto& visibles_forward = *visibles_forward_ptr;
            for (const auto& visible : visibles_forward)
            {
                std::shared_ptr<GameObject> gameobject_ptr = visible.lock();
                if (!gameobject_ptr)
                    continue;

                std::shared_ptr<ModelComponent> model_ptr =
                    gameobject_ptr->TryGetComponent<ModelComponent>("ModelComponent");
                if (!model_ptr)
                    continue;

                auto model_res_ptr = model_ptr->model_ptr.lock();
                if (!model_res_ptr)
                    continue;

                m_forward_mat.BindDescriptorSetToPipeline(command_buffer, 1, 1);

                for (uint32_t i = 0; i < model_res_ptr->meshes.size(); ++i)
                {
                    m_forward_mat.BindDescriptorSetToPipeline(command_buffer, 2, 1, draw_call[0], true);
                    model_res_ptr->meshes[i]->BindDrawCmd(command_buffer);

                    ++draw_call[0];
                }
            }
        }
    }

    void ForwardPass::RenderSkybox(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_skybox_mat.BindDescriptorSetToPipeline(command_buffer, 0, 2);

        m_skybox_model.meshes[0]->BindDrawCmd(command_buffer);
        ++draw_call[1];
    }

    void swap(ForwardPass& lhs, ForwardPass& rhs)
    {
        using std::swap;

        swap(lhs.m_forward_mat, rhs.m_forward_mat);
        swap(lhs.m_skybox_mat, rhs.m_skybox_mat);
        swap(lhs.m_skybox_model, rhs.m_skybox_model);
        swap(lhs.m_translucent_mat, rhs.m_translucent_mat);

        swap(lhs.m_pass_names, rhs.m_pass_names);
        swap(lhs.draw_call, rhs.draw_call);
    }
} // namespace Meow
