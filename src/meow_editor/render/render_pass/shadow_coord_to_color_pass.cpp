#include "shadow_coord_to_color_pass.h"

#include "pch.h"

#include "core/math/math.h"
#include "function/components/light/directional_light_component.h"
#include "function/components/model/model_component.h"
#include "function/global/runtime_context.h"
#include "function/render/buffer_data/per_scene_data.h"
#include "function/render/geometry/geometry_factory.h"
#include "function/render/material/material_factory.h"
#include "function/render/utils/vulkan_debug_utils.h"
#include "global/editor_context.h"

namespace Meow
{
    ShadowCoordToColorPass::ShadowCoordToColorPass(SurfaceData& surface_data)
        : RenderPassBase(surface_data)
    {
        CreateRenderPass();
        CreateMaterial();
    }

    void ShadowCoordToColorPass::CreateRenderPass()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // Create a set to store all information of attachments

        std::vector<vk::AttachmentDescription> attachment_descriptions;

        attachment_descriptions = {
            // shadow coord color attachment
            {
                vk::AttachmentDescriptionFlags(),        /* flags */
                m_color_format,                          /* format */
                vk::SampleCountFlagBits::e1,             /* samples */
                vk::AttachmentLoadOp::eClear,            /* loadOp */
                vk::AttachmentStoreOp::eStore,           /* storeOp */
                vk::AttachmentLoadOp::eDontCare,         /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare,        /* stencilStoreOp */
                vk::ImageLayout::eUndefined,             /* initialLayout */
                vk::ImageLayout::eShaderReadOnlyOptimal, /* finalLayout */
            },
            // shadow depth color attachment
            {
                vk::AttachmentDescriptionFlags(),        /* flags */
                m_color_format,                          /* format */
                vk::SampleCountFlagBits::e1,             /* samples */
                vk::AttachmentLoadOp::eClear,            /* loadOp */
                vk::AttachmentStoreOp::eStore,           /* storeOp */
                vk::AttachmentLoadOp::eDontCare,         /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare,        /* stencilStoreOp */
                vk::ImageLayout::eUndefined,             /* initialLayout */
                vk::ImageLayout::eShaderReadOnlyOptimal, /* finalLayout */
            },
            // depth attachment
            {
                vk::AttachmentDescriptionFlags(),                /* flags */
                m_depth_format,                                  /* format */
                vk::SampleCountFlagBits::e1,                     /* samples */
                vk::AttachmentLoadOp::eClear,                    /* loadOp */
                vk::AttachmentStoreOp::eDontCare,                /* storeOp */
                vk::AttachmentLoadOp::eDontCare,                 /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare,                /* stencilStoreOp */
                vk::ImageLayout::eUndefined,                     /* initialLayout */
                vk::ImageLayout::eDepthStencilAttachmentOptimal, /* finalLayout */
            },
        };
        std::vector<vk::AttachmentReference> color_attachment_references {
            {0, vk::ImageLayout::eColorAttachmentOptimal},
            {1, vk::ImageLayout::eColorAttachmentOptimal},
        };
        vk::AttachmentReference depth_attachment_reference(2, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        std::vector<vk::SubpassDescription> subpass_descriptions {
            // depth to color pass
            {
                vk::SubpassDescriptionFlags(),      /* flags */
                vk::PipelineBindPoint::eGraphics,   /* pipelineBindPoint */
                0,                                  /* inputAttachmentCount */
                nullptr,                            /* pInputAttachments */
                2,                                  /* colorAttachmentCount */
                color_attachment_references.data(), /* pColorAttachments */
                nullptr,                            /* pResolveAttachments */
                &depth_attachment_reference,        /* pDepthStencilAttachment */
                0,                                  /* preserveAttachmentCount */
                nullptr,                            /* pPreserveAttachments */
            },
        };

        std::vector<vk::SubpassDependency> dependencies {
            // externel -> depth to color pass
            {
                VK_SUBPASS_EXTERNAL,                               /* srcSubpass */
                0,                                                 /* dstSubpass */
                vk::PipelineStageFlagBits::eBottomOfPipe,          /* srcStageMask */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dstStageMask */
                vk::AccessFlagBits::eMemoryRead,                   /* srcAccessMask */
                vk::AccessFlagBits::eColorAttachmentWrite,         /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                 /* dependencyFlags */
            },
            // depth to color pass -> externel
            {
                0,                                                 /* srcSubpass */
                VK_SUBPASS_EXTERNAL,                               /* dstSubpass */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                vk::PipelineStageFlagBits::eBottomOfPipe,          /* dstStageMask */
                vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                vk::AccessFlagBits::eMemoryRead,                   /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                 /* dependencyFlags */
            },
        };

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(), /* flags */
                                                         attachment_descriptions,     /* pAttachments */
                                                         subpass_descriptions,        /* pSubpasses */
                                                         dependencies);               /* pDependencies */

        render_pass = vk::raii::RenderPass(logical_device, render_pass_create_info);

        clear_values.resize(3);
        clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[1].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[2].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::DebugUtilsObjectNameInfoEXT name_info = {vk::ObjectType::eRenderPass,
                                                     NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkRenderPass, *render_pass),
                                                     "Shadow Coord To Color RenderPass"};
        logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void ShadowCoordToColorPass::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        MaterialFactory material_factory;

        auto shadow_coord_to_color_shader = std::make_shared<Shader>(physical_device,
                                                                     logical_device,
                                                                     "builtin/shaders/shadow_coord_to_color.vert.spv",
                                                                     "builtin/shaders/shadow_coord_to_color.frag.spv");
        m_shadow_coord_to_color_material  = std::make_shared<Material>(shadow_coord_to_color_shader);
        g_runtime_context.resource_system->Register(m_shadow_coord_to_color_material);
        material_factory.Init(shadow_coord_to_color_shader.get(), vk::FrontFace::eClockwise);
        material_factory.SetOpaque(true, 2);
        material_factory.CreatePipeline(
            logical_device, render_pass, shadow_coord_to_color_shader.get(), m_shadow_coord_to_color_material.get(), 0);
        m_shadow_coord_to_color_material->SetDebugName("Shadow Coord to Color Material");

        // Create quad model
        std::vector<float>    vertices = {-1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
                                          1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f};
        std::vector<uint32_t> indices  = {0, 1, 2, 0, 2, 3};

        m_quad_model = std::move(Model(
            std::move(vertices), std::move(indices), m_shadow_coord_to_color_material->shader->per_vertex_attributes));
    }

    void ShadowCoordToColorPass::RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                                     const vk::Extent2D&               extent)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // clear

        framebuffers.clear();

        m_shadow_coord_to_color_render_target = nullptr;
        m_shadow_depth_to_color_render_target = nullptr;

        // Create attachment

        m_shadow_coord_to_color_render_target = ImageData::CreateRenderTarget(
            m_color_format,
            extent,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment,
            vk::ImageAspectFlagBits::eColor,
            {},
            false);

        m_shadow_depth_to_color_render_target = ImageData::CreateRenderTarget(
            m_color_format,
            extent,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment,
            vk::ImageAspectFlagBits::eColor,
            {},
            false);

        m_shadow_coord_to_color_render_target->SetDebugName("Shadow Coord to Color Render Target");
        m_shadow_depth_to_color_render_target->SetDebugName("Shadow Depth to Color Render Target");

        vk::ImageView attachments[3];
        attachments[0] = *m_shadow_coord_to_color_render_target->image_view;
        attachments[1] = *m_shadow_depth_to_color_render_target->image_view;
        attachments[2] = *m_depth_debugging_attachment->image_view;

        // Provide attachment information to frame buffer
        vk::FramebufferCreateInfo framebuffer_create_info(
            vk::FramebufferCreateFlags(),                         /* flags */
            *render_pass,                                         /* renderPass */
            3,                                                    /* attachmentCount */
            attachments,                                          /* pAttachments */
            m_shadow_coord_to_color_render_target->extent.width,  /* width */
            m_shadow_coord_to_color_render_target->extent.height, /* height */
            1);                                                   /* layers */

        framebuffers.reserve(output_image_views.size());
        for (const auto& imageView : output_image_views)
        {
            framebuffers.push_back(vk::raii::Framebuffer(logical_device, framebuffer_create_info));
        }
    }

    void ShadowCoordToColorPass::UpdateUniformBuffer()
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

        m_shadow_coord_to_color_material->PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));

        // light

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
                                         (float)m_shadow_coord_to_color_render_target->extent.width /
                                             m_shadow_coord_to_color_render_target->extent.height,
                                         directional_light_comp_ptr->near_plane,
                                         directional_light_comp_ptr->far_plane);
                m_shadow_coord_to_color_material->PopulateUniformBuffer(
                    "lightData", &per_light_data, sizeof(per_light_data));
                break;
            }
        }

        m_shadow_coord_to_color_material->BeginPopulatingDynamicUniformBufferPerFrame();
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
                    m_shadow_coord_to_color_material->BeginPopulatingDynamicUniformBufferPerObject();
                    m_shadow_coord_to_color_material->PopulateDynamicUniformBuffer("objData", &model, sizeof(model));
                    m_shadow_coord_to_color_material->EndPopulatingDynamicUniformBufferPerObject();
                }
            }
            m_shadow_coord_to_color_material->EndPopulatingDynamicUniformBufferPerFrame();
        }
    }

    void ShadowCoordToColorPass::Start(const vk::raii::CommandBuffer& command_buffer,
                                       vk::Extent2D                   extent,
                                       uint32_t                       current_image_index)
    {
        for (int i = 0; i < 1; i++)
        {
            draw_call[i] = 0;
        }

        RenderPassBase::Start(command_buffer, m_shadow_coord_to_color_render_target->extent, current_image_index);

        command_buffer.setViewport(0,
                                   vk::Viewport(0.0f,
                                                static_cast<float>(extent.height),
                                                static_cast<float>(extent.width),
                                                -static_cast<float>(extent.height),
                                                0.0f,
                                                1.0f));
        command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
    }

    void ShadowCoordToColorPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_shadow_coord_to_color_material->BindPipeline(command_buffer);
        m_shadow_coord_to_color_material->BindDescriptorSetToPipeline(command_buffer, 0, 1);

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
                    m_shadow_coord_to_color_material->BindDescriptorSetToPipeline(
                        command_buffer, 1, 1, draw_call[0], true);
                    model_resource->meshes[i]->BindDrawCmd(command_buffer);

                    ++draw_call[0];
                }
            }
        }
    }

    void swap(ShadowCoordToColorPass& lhs, ShadowCoordToColorPass& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPassBase&>(lhs), static_cast<RenderPassBase&>(rhs));

        swap(lhs.m_shadow_coord_to_color_material, rhs.m_shadow_coord_to_color_material);
        swap(lhs.m_shadow_coord_to_color_render_target, rhs.m_shadow_coord_to_color_render_target);
        swap(lhs.m_shadow_depth_to_color_render_target, rhs.m_shadow_depth_to_color_render_target);
        swap(lhs.m_quad_model, rhs.m_quad_model);
    }
} // namespace Meow
