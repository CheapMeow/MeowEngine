#include "shadow_map_pass.h"

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

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    void ShadowMapPass::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        MaterialFactory material_factory;

        auto shadow_map_shader = std::make_shared<Shader>(physical_device,
                                                          logical_device,
                                                          "builtin/shaders/shadow_map.vert.spv",
                                                          "builtin/shaders/shadow_map.frag.spv");

        m_shadow_map_material = std::make_shared<Material>(shadow_map_shader);
        g_runtime_context.resource_system->Register(m_shadow_map_material);
        material_factory.Init(shadow_map_shader.get(), vk::FrontFace::eClockwise);
        material_factory.SetOpaque(true, 0);
        material_factory.SetDebugName("Forward Shadow Map Material");
        material_factory.CreatePipeline(
            logical_device, render_pass, shadow_map_shader.get(), m_shadow_map_material.get(), 0);

        m_shadow_map = ImageData::CreateRenderTarget(m_depth_format,
                                                     {2048, 2048},
                                                     vk::ImageUsageFlagBits::eDepthStencilAttachment |
                                                         vk::ImageUsageFlagBits::eInputAttachment,
                                                     vk::ImageAspectFlagBits::eDepth,
                                                     {},
                                                     false);
    }

    void ShadowMapPass::RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                            const vk::Extent2D&               extent)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // clear

        framebuffers.clear();

        // Create attachment

        // Provide attachment information to frame buffer

        vk::FramebufferCreateInfo framebuffer_create_info(vk::FramebufferCreateFlags(), /* flags */
                                                          *render_pass,                 /* renderPass */
                                                          1,                            /* attachmentCount */
                                                          &(*m_shadow_map->image_view), /* pAttachments */
                                                          m_shadow_map->extent.width,   /* width */
                                                          m_shadow_map->extent.height,  /* height */
                                                          1);                           /* layers */

        framebuffers.reserve(output_image_views.size());
        for (const auto& imageView : output_image_views)
        {
            framebuffers.push_back(vk::raii::Framebuffer(logical_device, framebuffer_create_info));
        }
    }

    void ShadowMapPass::UpdateUniformBuffer()
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

        glm::vec3 forward = main_camera_transfrom_component->rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        glm::mat4 view    = lookAt(main_camera_transfrom_component->position,
                                main_camera_transfrom_component->position + forward,
                                glm::vec3(0.0f, 1.0f, 0.0f));

        const auto* visibles_opaque_ptr = level->GetVisiblesPerShadingModel(ShadingModelType::Opaque);

        // Shadow map

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
                                         (float)m_shadow_map->extent.width / m_shadow_map->extent.height,
                                         directional_light_comp_ptr->near_plane,
                                         directional_light_comp_ptr->far_plane);
                m_shadow_map_material->PopulateUniformBuffer("lightData", &per_light_data, sizeof(per_light_data));
                break;
            }
        }

        m_shadow_map_material->BeginPopulatingDynamicUniformBufferPerFrame();
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
                    m_shadow_map_material->BeginPopulatingDynamicUniformBufferPerObject();
                    m_shadow_map_material->PopulateDynamicUniformBuffer("objData", &model, sizeof(model));
                    m_shadow_map_material->EndPopulatingDynamicUniformBufferPerObject();
                }
            }
            m_shadow_map_material->EndPopulatingDynamicUniformBufferPerFrame();
        }
    }

    void ShadowMapPass::Start(const vk::raii::CommandBuffer& command_buffer,
                              vk::Extent2D                   extent,
                              uint32_t                       current_image_index)
    {
        draw_call[0] = 0;

        RenderPass::Start(command_buffer, m_shadow_map->extent, current_image_index);

        command_buffer.setViewport(0,
                                   vk::Viewport(0.0f,
                                                static_cast<float>(m_shadow_map->extent.height),
                                                static_cast<float>(m_shadow_map->extent.width),
                                                -static_cast<float>(m_shadow_map->extent.height),
                                                0.0f,
                                                1.0f));
        command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_shadow_map->extent));
    }

    void ShadowMapPass::RenderShadowMap(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_shadow_map_material->BindDescriptorSetToPipeline(command_buffer, 0, 1);

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
                    m_shadow_map_material->BindDescriptorSetToPipeline(command_buffer, 1, 1, draw_call[0], true);
                    model_resource->meshes[i]->BindDrawCmd(command_buffer);

                    ++draw_call[0];
                }
            }
        }
    }

    void swap(ShadowMapPass& lhs, ShadowMapPass& rhs)
    {
        using std::swap;

        swap(lhs.m_shadow_map_material, rhs.m_shadow_map_material);
        swap(lhs.m_shadow_map, rhs.m_shadow_map);

        swap(lhs.draw_call, rhs.draw_call);
    }
} // namespace Meow
