#include "editor_deferred_pass.h"

#include "pch.h"

#include "global/editor_context.h"
#include "meow_runtime/core/math/math.h"
#include "meow_runtime/function/components/camera/camera_3d_component.hpp"
#include "meow_runtime/function/components/model/model_component.h"
#include "meow_runtime/function/components/transform/transform_3d_component.hpp"
#include "meow_runtime/function/global/runtime_context.h"
#include "meow_runtime/function/object/game_object.h"
#include "meow_runtime/function/render/structs/ubo_data.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <imgui.h>

namespace Meow
{
    EditorDeferredPass::EditorDeferredPass(const vk::raii::PhysicalDevice& physical_device,
                                           const vk::raii::Device&         device,
                                           SurfaceData&                    surface_data,
                                           const vk::raii::CommandPool&    command_pool,
                                           const vk::raii::Queue&          queue,
                                           DescriptorAllocatorGrowable&    m_descriptor_allocator)
        : RenderPass(device)
    {
        m_pass_name = "Deferred Pass";

        m_pass_names[0] = m_pass_name + " - Obj2Attachment Subpass";
        m_pass_names[1] = m_pass_name + " - Quad Subpass";

        // Create a set to store all information of attachments

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        std::vector<vk::AttachmentDescription> attachment_descriptions;
        // swap chain attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(), /* flags */
                                             color_format,                     /* format */
                                             m_sample_count,                   /* samples */
                                             vk::AttachmentLoadOp::eClear,     /* loadOp */
                                             vk::AttachmentStoreOp::eStore,    /* storeOp */
                                             vk::AttachmentLoadOp::eDontCare,  /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eDontCare, /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,      /* initialLayout */
                                             vk::ImageLayout::ePresentSrcKHR); /* finalLayout */
        // color attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),          /* flags */
                                             color_format,                              /* format */
                                             m_sample_count,                            /* samples */
                                             vk::AttachmentLoadOp::eClear,              /* loadOp */
                                             vk::AttachmentStoreOp::eStore,             /* storeOp */
                                             vk::AttachmentLoadOp::eDontCare,           /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eDontCare,          /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,               /* initialLayout */
                                             vk::ImageLayout::eColorAttachmentOptimal); /* finalLayout */
        // normal attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),          /* flags */
                                             vk::Format::eR8G8B8A8Unorm,                /* format */
                                             m_sample_count,                            /* samples */
                                             vk::AttachmentLoadOp::eClear,              /* loadOp */
                                             vk::AttachmentStoreOp::eStore,             /* storeOp */
                                             vk::AttachmentLoadOp::eDontCare,           /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eDontCare,          /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,               /* initialLayout */
                                             vk::ImageLayout::eColorAttachmentOptimal); /* finalLayout */
        // position attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),          /* flags */
                                             vk::Format::eR16G16B16A16Sfloat,           /* format */
                                             m_sample_count,                            /* samples */
                                             vk::AttachmentLoadOp::eClear,              /* loadOp */
                                             vk::AttachmentStoreOp::eStore,             /* storeOp */
                                             vk::AttachmentLoadOp::eDontCare,           /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eDontCare,          /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,               /* initialLayout */
                                             vk::ImageLayout::eColorAttachmentOptimal); /* finalLayout */
        // depth attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),                 /* flags */
                                             m_depth_format,                                   /* format */
                                             m_sample_count,                                   /* samples */
                                             vk::AttachmentLoadOp::eClear,                     /* loadOp */
                                             vk::AttachmentStoreOp::eStore,                    /* storeOp */
                                             vk::AttachmentLoadOp::eClear,                     /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eStore,                    /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,                      /* initialLayout */
                                             vk::ImageLayout::eDepthStencilAttachmentOptimal); /* finalLayout */

        // Create reference to attachment information set

        vk::AttachmentReference swapchain_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

        std::vector<vk::AttachmentReference> color_attachment_references;
        color_attachment_references.emplace_back(1, vk::ImageLayout::eColorAttachmentOptimal);
        color_attachment_references.emplace_back(2, vk::ImageLayout::eColorAttachmentOptimal);
        color_attachment_references.emplace_back(3, vk::ImageLayout::eColorAttachmentOptimal);

        vk::AttachmentReference depth_attachment_reference(4, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        std::vector<vk::AttachmentReference> input_attachment_references;
        input_attachment_references.emplace_back(1, vk::ImageLayout::eShaderReadOnlyOptimal);
        input_attachment_references.emplace_back(2, vk::ImageLayout::eShaderReadOnlyOptimal);
        input_attachment_references.emplace_back(3, vk::ImageLayout::eShaderReadOnlyOptimal);
        input_attachment_references.emplace_back(4, vk::ImageLayout::eShaderReadOnlyOptimal);

        // Create subpass

        std::vector<vk::SubpassDescription> subpass_descriptions;
        // obj2attachment pass
        subpass_descriptions.push_back(vk::SubpassDescription(vk::SubpassDescriptionFlags(),    /* flags */
                                                              vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                                                              {},                               /* pInputAttachments */
                                                              color_attachment_references,      /* pColorAttachments */
                                                              {},                          /* pResolveAttachments */
                                                              &depth_attachment_reference, /* pDepthStencilAttachment */
                                                              nullptr));                   /* pPreserveAttachments */
        // quad renderering pass
        subpass_descriptions.push_back(vk::SubpassDescription(vk::SubpassDescriptionFlags(),    /* flags */
                                                              vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                                                              input_attachment_references,      /* pInputAttachments */
                                                              swapchain_attachment_reference,   /* pColorAttachments */
                                                              {},        /* pResolveAttachments */
                                                              {},        /* pDepthStencilAttachment */
                                                              nullptr)); /* pPreserveAttachments */

        // Create subpass dependency

        std::vector<vk::SubpassDependency> dependencies;
        // externel -> obj2attachment pass
        dependencies.emplace_back(VK_SUBPASS_EXTERNAL,                               /* srcSubpass */
                                  0,                                                 /* dstSubpass */
                                  vk::PipelineStageFlagBits::eBottomOfPipe,          /* srcStageMask */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dstStageMask */
                                  vk::AccessFlagBits::eMemoryRead,                   /* srcAccessMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite,         /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion);                /* dependencyFlags */
        // obj2attachment pass -> quad renderering pass
        dependencies.emplace_back(0,                                                 /* srcSubpass */
                                  1,                                                 /* dstSubpass */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                                  vk::PipelineStageFlagBits::eFragmentShader,        /* dstStageMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                                  vk::AccessFlagBits::eShaderRead,                   /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion);                /* dependencyFlags */
        // quad renderering pass -> externel
        dependencies.emplace_back(1,                                                 /* srcSubpass */
                                  VK_SUBPASS_EXTERNAL,                               /* dstSubpass */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                                  vk::PipelineStageFlagBits::eFragmentShader,        /* dstStageMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                                  vk::AccessFlagBits::eShaderRead,                   /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion);                /* dependencyFlags */

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(), /* flags */
                                                         attachment_descriptions,     /* pAttachments */
                                                         subpass_descriptions,        /* pSubpasses */
                                                         dependencies);               /* pDependencies */

        render_pass = vk::raii::RenderPass(device, render_pass_create_info);

        // Create Material

        auto obj_shader_ptr = std::make_shared<Shader>(physical_device,
                                                       device,
                                                       m_descriptor_allocator,
                                                       "builtin/shaders/obj.vert.spv",
                                                       "builtin/shaders/obj.frag.spv");

        m_obj2attachment_mat                        = Material(physical_device, device, obj_shader_ptr);
        m_obj2attachment_mat.color_attachment_count = 3;
        m_obj2attachment_mat.CreatePipeline(device, render_pass, vk::FrontFace::eClockwise, true);

        auto quad_shader_ptr = std::make_shared<Shader>(physical_device,
                                                        device,
                                                        m_descriptor_allocator,
                                                        "builtin/shaders/quad.vert.spv",
                                                        "builtin/shaders/quad.frag.spv");

        m_quad_mat         = Material(physical_device, device, quad_shader_ptr);
        m_quad_mat.subpass = 1;
        m_quad_mat.CreatePipeline(device, render_pass, vk::FrontFace::eClockwise, false);

        m_render_stat[0].vertex_attribute_metas = m_obj2attachment_mat.shader_ptr->vertex_attribute_metas;
        m_render_stat[0].buffer_meta_map        = m_obj2attachment_mat.shader_ptr->buffer_meta_map;
        m_render_stat[0].image_meta_map         = m_obj2attachment_mat.shader_ptr->image_meta_map;

        m_render_stat[1].vertex_attribute_metas = m_quad_mat.shader_ptr->vertex_attribute_metas;
        m_render_stat[1].buffer_meta_map        = m_quad_mat.shader_ptr->buffer_meta_map;
        m_render_stat[1].image_meta_map         = m_quad_mat.shader_ptr->image_meta_map;

        // Create quad model
        std::vector<float>    vertices = {-1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
                                          1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f};
        std::vector<uint32_t> indices  = {0, 1, 2, 0, 2, 3};

        m_quad_model = std::move(Model(physical_device,
                                       device,
                                       command_pool,
                                       queue,
                                       std::move(vertices),
                                       std::move(indices),
                                       m_quad_mat.shader_ptr->per_vertex_attributes));

        clear_values.resize(5);
        clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[1].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[2].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[3].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[4].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        input_vertex_attributes = m_obj2attachment_mat.shader_ptr->per_vertex_attributes;

        // Debug

        VkQueryPoolCreateInfo query_pool_create_info = {.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                                        .queryType          = VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                                        .queryCount         = 2,
                                                        .pipelineStatistics = (1 << 11) - 1};

        query_pool = device.createQueryPool(query_pool_create_info, nullptr);

        // init light

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
            m_LightInfos.direction[i] = glm::normalize(m_LightInfos.direction[i]);
            m_LightInfos.speed[i]     = 1.0f + glm::linearRand<float>(1.0f, 2.0f);
        }
    }

    void EditorDeferredPass::RefreshFrameBuffers(const vk::raii::PhysicalDevice&         physical_device,
                                                 const vk::raii::Device&                 device,
                                                 const vk::raii::CommandPool&            command_pool,
                                                 const vk::raii::Queue&                  queue,
                                                 SurfaceData&                            surface_data,
                                                 const std::vector<vk::raii::ImageView>& swapchain_image_views,
                                                 const vk::Extent2D&                     extent)
    {
        // clear

        framebuffers.clear();

        m_color_attachment    = nullptr;
        m_normal_attachment   = nullptr;
        m_position_attachment = nullptr;
        m_depth_attachment    = nullptr;

        // Create attachment

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;

        m_color_attachment = ImageData::CreateAttachment(physical_device,
                                                         device,
                                                         command_pool,
                                                         queue,
                                                         color_format,
                                                         extent,
                                                         vk::ImageUsageFlagBits::eColorAttachment |
                                                             vk::ImageUsageFlagBits::eInputAttachment,
                                                         vk::ImageAspectFlagBits::eColor,
                                                         {},
                                                         false);

        m_normal_attachment = ImageData::CreateAttachment(physical_device,
                                                          device,
                                                          command_pool,
                                                          queue,
                                                          vk::Format::eR8G8B8A8Unorm,
                                                          extent,
                                                          vk::ImageUsageFlagBits::eColorAttachment |
                                                              vk::ImageUsageFlagBits::eInputAttachment,
                                                          vk::ImageAspectFlagBits::eColor,
                                                          {},
                                                          false);

        m_position_attachment = ImageData::CreateAttachment(physical_device,
                                                            device,
                                                            command_pool,
                                                            queue,
                                                            vk::Format::eR16G16B16A16Sfloat,
                                                            extent,
                                                            vk::ImageUsageFlagBits::eColorAttachment |
                                                                vk::ImageUsageFlagBits::eInputAttachment,
                                                            vk::ImageAspectFlagBits::eColor,
                                                            {},
                                                            false);

        m_depth_attachment = ImageData::CreateAttachment(physical_device,
                                                         device,
                                                         command_pool,
                                                         queue,
                                                         m_depth_format,
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

        framebuffers.reserve(swapchain_image_views.size());
        for (const auto& imageView : swapchain_image_views)
        {
            attachments[0] = *imageView;
            framebuffers.push_back(vk::raii::Framebuffer(device, framebuffer_create_info));
        }

        // Update descriptor set

        m_quad_mat.SetImage(device, "inputColor", *m_color_attachment);
        m_quad_mat.SetImage(device, "inputNormal", *m_normal_attachment);
        m_quad_mat.SetImage(device, "inputPosition", *m_position_attachment);
        m_quad_mat.SetImage(device, "inputDepth", *m_depth_attachment);
    }

    void EditorDeferredPass::UpdateUniformBuffer()
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

        m_obj2attachment_mat.BeginFrame();
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

            // ubo_data.model = glm::rotate(ubo_data.model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            for (int32_t i = 0; i < model_comp_ptr->model_ptr.lock()->meshes.size(); ++i)
            {
                m_obj2attachment_mat.BeginObject();
                m_obj2attachment_mat.SetLocalUniformBuffer("uboMVP", &ubo_data, sizeof(ubo_data));
                m_obj2attachment_mat.EndObject();
            }
        }
        m_obj2attachment_mat.EndFrame();

        // update light

        for (int32_t i = 0; i < k_num_lights; ++i)
        {
            float bias = glm::sin(g_runtime_context.time_system->GetTime() * m_LightInfos.speed[i]) / 5.0f;
            m_LightDatas.lights[i].position.x = m_LightInfos.position[i].x + bias * m_LightInfos.direction[i].x;
            m_LightDatas.lights[i].position.y = m_LightInfos.position[i].y + bias * m_LightInfos.direction[i].y;
            m_LightDatas.lights[i].position.z = m_LightInfos.position[i].z + bias * m_LightInfos.direction[i].z;
        }

        m_quad_mat.BeginFrame();
        m_quad_mat.BeginObject();
        m_quad_mat.SetLocalUniformBuffer("lightDatas", &m_LightDatas, sizeof(m_LightDatas));
        m_quad_mat.EndObject();
        m_quad_mat.EndFrame();
    }

    void EditorDeferredPass::Start(const vk::raii::CommandBuffer& command_buffer,
                                   const Meow::SurfaceData&       surface_data,
                                   uint32_t                       current_image_index)
    {
        // Debug
        if (m_query_enabled)
        {
            for (int i = 0; i < 2; i++)
            {
                command_buffer.resetQueryPool(*query_pool, 0, 2);
            }
        }

        RenderPass::Start(command_buffer, surface_data, current_image_index);

        draw_call = 0;
    }

    void EditorDeferredPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_obj2attachment_mat.BindPipeline(command_buffer);

        // Debug
        if (m_query_enabled)
            command_buffer.beginQuery(*query_pool, 0, {});

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
                m_obj2attachment_mat.BindDescriptorSets(command_buffer, m_render_stat[0].draw_call);
                model_comp_ptr->model_ptr.lock()->meshes[i]->BindDrawCmd(command_buffer);

                ++m_render_stat[0].draw_call;
            }
        }

        // Debug
        if (m_query_enabled)
            command_buffer.endQuery(*query_pool, 0);

        command_buffer.nextSubpass(vk::SubpassContents::eInline);

        m_quad_mat.BindPipeline(command_buffer);

        // Debug
        if (m_query_enabled)
            command_buffer.beginQuery(*query_pool, 1, {});

        for (int32_t i = 0; i < m_quad_model.meshes.size(); ++i)
        {
            m_quad_mat.BindDescriptorSets(command_buffer, m_render_stat[1].draw_call);
            m_quad_model.meshes[i]->BindDrawCmd(command_buffer);

            ++m_render_stat[1].draw_call;
        }

        // Debug
        if (m_query_enabled)
            command_buffer.endQuery(*query_pool, 1);
    }

    void EditorDeferredPass::AfterPresent()
    {
        FUNCTION_TIMER();

        if (m_query_enabled)
        {
            for (int i = 1; i >= 0; i--)
            {
                std::pair<vk::Result, std::vector<uint32_t>> query_results =
                    query_pool.getResults<uint32_t>(i, 1, sizeof(uint32_t) * 11, sizeof(uint32_t) * 11, {});

                g_editor_context.profile_system->UploadPipelineStat(m_pass_names[i], query_results.second);
            }
        }

        m_render_stat[0].ringbuf_stat.populate(m_obj2attachment_mat.GetRingUniformBuffer());
        m_render_stat[1].ringbuf_stat.populate(m_quad_mat.GetRingUniformBuffer());

        for (int i = 1; i >= 0; i--)
        {
            m_render_stat[i].draw_call = draw_call;
            g_editor_context.profile_system->UploadBuiltinRenderStat(m_pass_names[i], m_render_stat[i]);
        }
    }

    void swap(EditorDeferredPass& lhs, EditorDeferredPass& rhs)
    {
        using std::swap;

        swap(lhs.m_obj2attachment_mat, rhs.m_obj2attachment_mat);
        swap(lhs.m_quad_mat, rhs.m_quad_mat);
        swap(lhs.m_quad_model, rhs.m_quad_model);

        swap(lhs.m_color_attachment, rhs.m_color_attachment);
        swap(lhs.m_normal_attachment, rhs.m_normal_attachment);
        swap(lhs.m_position_attachment, rhs.m_position_attachment);

        swap(lhs.m_LightDatas, rhs.m_LightDatas);
        swap(lhs.m_LightInfos, rhs.m_LightInfos);

        swap(lhs.m_pass_names, rhs.m_pass_names);
        swap(lhs.m_render_stat, rhs.m_render_stat);
    }
} // namespace Meow
