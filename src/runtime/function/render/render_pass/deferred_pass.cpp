#include "deferred_pass.h"

#include "core/time/time.h"
#include "function/components/3d/camera/camera_3d_component.h"
#include "function/components/3d/model/model_component.h"
#include "function/components/3d/transform/transform_3d_component.h"
#include "function/global/runtime_global_context.h"
#include "function/render/structs/ubo_data.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <imgui.h>

namespace Meow
{
    DeferredPass::DeferredPass(vk::raii::PhysicalDevice const& physical_device,
                               vk::raii::Device const&         device,
                               SurfaceData&                    surface_data,
                               vk::raii::CommandPool const&    command_pool,
                               vk::raii::Queue const&          queue,
                               DescriptorAllocatorGrowable&    m_descriptor_allocator)
        : RenderPass(nullptr)
    {
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

        std::shared_ptr<Shader> obj_shader_ptr = std::make_shared<Shader>(physical_device,
                                                                          device,
                                                                          m_descriptor_allocator,
                                                                          "builtin/shaders/obj.vert.spv",
                                                                          "builtin/shaders/obj.frag.spv");

        m_obj2attachment_mat                        = Material(physical_device, device, obj_shader_ptr);
        m_obj2attachment_mat.color_attachment_count = 3;
        m_obj2attachment_mat.CreatePipeline(device, render_pass, vk::FrontFace::eClockwise, true);

        std::shared_ptr<Shader> quad_shader_ptr = std::make_shared<Shader>(physical_device,
                                                                           device,
                                                                           m_descriptor_allocator,
                                                                           "builtin/shaders/quad.vert.spv",
                                                                           "builtin/shaders/quad.frag.spv");

        m_quad_mat         = Material(physical_device, device, quad_shader_ptr);
        m_quad_mat.subpass = 1;
        m_quad_mat.CreatePipeline(device, render_pass, vk::FrontFace::eClockwise, false);

        // Create quad model
        std::vector<float>    vertices = {-1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
                                          1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f};
        std::vector<uint16_t> indices  = {0, 1, 2, 0, 2, 3};

        m_quad_model = std::move(Model(physical_device,
                                       device,
                                       command_pool,
                                       queue,
                                       vertices,
                                       indices,
                                       m_quad_mat.shader_ptr->per_vertex_attributes));

        clear_values.resize(5);
        clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[1].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[2].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[3].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[4].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        input_vertex_attributes = m_obj2attachment_mat.shader_ptr->per_vertex_attributes;

        // debug

        VkQueryPoolCreateInfo query_pool_create_info = {.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                                        .queryType          = VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                                        .queryCount         = 1,
                                                        .pipelineStatistics = (1 << statisticCount) - 1};

        enable_query = true;
        query_pool   = device.createQueryPool(query_pool_create_info, nullptr);

        // init light

        for (int32_t i = 0; i < k_num_lights; ++i)
        {
            m_LightDatas.lights[i].position.x = glm::linearRand<float>(-2.0f, 2.0f);
            m_LightDatas.lights[i].position.y = glm::linearRand<float>(-2.0f, 2.0f);
            m_LightDatas.lights[i].position.z = glm::linearRand<float>(-2.0f, 2.0f);

            m_LightDatas.lights[i].color.x = glm::linearRand<float>(0.0f, 0.5f);
            m_LightDatas.lights[i].color.y = glm::linearRand<float>(0.0f, 0.5f);
            m_LightDatas.lights[i].color.z = glm::linearRand<float>(0.0f, 0.5f);

            m_LightDatas.lights[i].radius = glm::linearRand<float>(0.0f, 2.0f);

            m_LightInfos.position[i]  = m_LightDatas.lights[i].position;
            m_LightInfos.direction[i] = m_LightInfos.position[i];
            m_LightInfos.direction[i] = glm::normalize(m_LightInfos.direction[i]);
            m_LightInfos.speed[i]     = 1.0f + glm::linearRand<float>(1.0f, 2.0f);
        }
    }

    void DeferredPass::RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                           vk::raii::Device const&                 device,
                                           vk::raii::CommandBuffer const&          command_buffer,
                                           SurfaceData&                            surface_data,
                                           std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                           vk::Extent2D const&                     extent)
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
                                                         command_buffer,
                                                         color_format,
                                                         extent,
                                                         vk::ImageUsageFlagBits::eColorAttachment |
                                                             vk::ImageUsageFlagBits::eInputAttachment,
                                                         vk::ImageAspectFlagBits::eColor,
                                                         {},
                                                         false);

        m_normal_attachment = ImageData::CreateAttachment(physical_device,
                                                          device,
                                                          command_buffer,
                                                          vk::Format::eR8G8B8A8Unorm,
                                                          extent,
                                                          vk::ImageUsageFlagBits::eColorAttachment |
                                                              vk::ImageUsageFlagBits::eInputAttachment,
                                                          vk::ImageAspectFlagBits::eColor,
                                                          {},
                                                          false);

        m_position_attachment = ImageData::CreateAttachment(physical_device,
                                                            device,
                                                            command_buffer,
                                                            vk::Format::eR16G16B16A16Sfloat,
                                                            extent,
                                                            vk::ImageUsageFlagBits::eColorAttachment |
                                                                vk::ImageUsageFlagBits::eInputAttachment,
                                                            vk::ImageAspectFlagBits::eColor,
                                                            {},
                                                            false);

        m_depth_attachment = ImageData::CreateAttachment(physical_device,
                                                         device,
                                                         command_buffer,
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
        for (auto const& imageView : swapchain_image_views)
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

    void DeferredPass::UpdateUniformBuffer()
    {
        UBOData ubo_data;

        for (auto [entity, transfrom_component, camera_component] :
             g_runtime_global_context.registry.view<const Transform3DComponent, Camera3DComponent>().each())
        {
            if (camera_component.is_main_camera)
            {
                glm::ivec2 window_size = g_runtime_global_context.window_system->m_window->GetSize();

                glm::mat4 view = glm::mat4(1.0f);
                view           = glm::mat4_cast(glm::conjugate(transfrom_component.rotation)) * view;
                view           = glm::translate(view, -transfrom_component.position);

                ubo_data.view       = view;
                ubo_data.projection = glm::perspectiveLH_ZO(camera_component.field_of_view,
                                                            (float)window_size[0] / (float)window_size[1],
                                                            camera_component.near_plane,
                                                            camera_component.far_plane);
                break;
            }
        }

        // Update mesh uniform

        m_obj2attachment_mat.BeginFrame();
        for (auto [entity, transfrom_component, model_component] :
             g_runtime_global_context.registry.view<const Transform3DComponent, ModelComponent>().each())
        {
            ubo_data.model = transfrom_component.GetTransform();
            ubo_data.model = glm::rotate(ubo_data.model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            for (int32_t i = 0; i < model_component.model.meshes.size(); ++i)
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
            float bias                        = glm::sin(Time::GetTime() * m_LightInfos.speed[i]) / 5.0f;
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

    void DeferredPass::Draw(vk::raii::CommandBuffer const& command_buffer)
    {
        m_obj2attachment_mat.BindPipeline(command_buffer);

        cur_query_count = 0;

        for (auto [entity, transfrom_component, model_component] :
             g_runtime_global_context.registry.view<const Transform3DComponent, ModelComponent>().each())
        {
            // debug
            if (cur_query_count == 0)
            {
                command_buffer.beginQuery(*query_pool, 0, {});
            }

            for (int32_t i = 0; i < model_component.model.meshes.size(); ++i)
            {
                m_obj2attachment_mat.BindDescriptorSets(command_buffer, i);
                model_component.model.meshes[i]->BindDrawCmd(command_buffer);
            }

            // debug
            if (cur_query_count == 0)
            {
                command_buffer.endQuery(*query_pool, 0);
                cur_query_count++;
            }
        }

        command_buffer.nextSubpass(vk::SubpassContents::eInline);

        m_quad_mat.BindPipeline(command_buffer);

        for (int32_t i = 0; i < m_quad_model.meshes.size(); ++i)
        {
            m_quad_mat.BindDescriptorSets(command_buffer, i);
            m_quad_model.meshes[i]->BindDrawCmd(command_buffer);
        }
    }

    void DeferredPass::AfterRenderPass()
    {
        std::pair<vk::Result, std::vector<uint32_t>> query_results =
            query_pool.getResults<uint32_t>(0, 1, sizeof(statistics), sizeof(statistics), {});
        invocationCount_fs_query_count = query_results.second[invocationCount_fs];
    }
} // namespace Meow
