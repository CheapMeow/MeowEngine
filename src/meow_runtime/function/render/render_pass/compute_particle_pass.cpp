#include "compute_particle_pass.h"

#include "pch.h"

#include "core/math/math.h"
#include "function/global/runtime_context.h"
#include "function/particle/gpu_particle_data_2d.h"
#include "function/render/material/material_factory.h"
#include "function/render/material/shader_factory.h"

namespace Meow
{
    ComputeParticlePass::ComputeParticlePass(SurfaceData& surface_data)
        : RenderPassBase(surface_data)
    {
        CreateRenderPass();
        CreateMaterial();
    }

    void ComputeParticlePass::CreateRenderPass()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // Create a set to store all information of attachments

        std::vector<vk::AttachmentDescription> attachment_descriptions;

        attachment_descriptions = {
            // depth attachment
            {
                vk::AttachmentDescriptionFlags(),        /* flags */
                m_color_format,                          /* format */
                vk::SampleCountFlagBits::e1,             /* samples */
                vk::AttachmentLoadOp::eDontCare,         /* loadOp */
                vk::AttachmentStoreOp::eStore,           /* storeOp */
                vk::AttachmentLoadOp::eDontCare,         /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare,        /* stencilStoreOp */
                vk::ImageLayout::eShaderReadOnlyOptimal, /* initialLayout */
                vk::ImageLayout::eShaderReadOnlyOptimal, /* finalLayout */
            },
        };

        vk::AttachmentReference color_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

        std::vector<vk::SubpassDescription> subpass_descriptions {
            // particle render pass
            {
                vk::SubpassDescriptionFlags(),    /* flags */
                vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                0,                                /* inputAttachmentCount */
                nullptr,                          /* pInputAttachments */
                1,                                /* colorAttachmentCount */
                &color_attachment_reference,      /* pColorAttachments */
                nullptr,                          /* pResolveAttachments */
                nullptr,                          /* pDepthStencilAttachment */
                0,                                /* preserveAttachmentCount */
                nullptr,                          /* pPreserveAttachments */
            },
        };

        std::vector<vk::SubpassDependency> dependencies {
            // externel -> particle render pass
            {
                VK_SUBPASS_EXTERNAL,                               /* srcSubpass */
                0,                                                 /* dstSubpass */
                vk::PipelineStageFlagBits::eBottomOfPipe,          /* srcStageMask */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dstStageMask */
                vk::AccessFlagBits::eMemoryRead,                   /* srcAccessMask */
                vk::AccessFlagBits::eColorAttachmentWrite,         /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                 /* dependencyFlags */
            },
            // particle render pass -> externel
            {
                0,                                                 /* srcSubpass */
                VK_SUBPASS_EXTERNAL,                               /* dstSubpass */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                vk::PipelineStageFlagBits::eBottomOfPipe,          /* dstStageMask */
                vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                vk::AccessFlagBits::eShaderRead,                   /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                 /* dependencyFlags */
            },
        };

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(), /* flags */
                                                         attachment_descriptions,     /* pAttachments */
                                                         subpass_descriptions,        /* pSubpasses */
                                                         dependencies);               /* pDependencies */

        render_pass = vk::raii::RenderPass(logical_device, render_pass_create_info);

        clear_values.resize(1);
        clear_values[0].color = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
    }

    void ComputeParticlePass::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        ShaderFactory   shader_factory;
        MaterialFactory material_factory;

        auto particle_render_shader = shader_factory.clear()
                                          .SetVertexShader("builtin/shaders/gpu_particle_2d.vert.spv")
                                          .SetFragmentShader("builtin/shaders/gpu_particle_2d.frag.spv")
                                          .Create();
        m_particle_render_material = std::make_shared<Material>(particle_render_shader);
        material_factory.Init(particle_render_shader.get(), vk::FrontFace::eClockwise);
        material_factory.SetVertexAttributeStrideAndOffset(
            sizeof(GPUParticleData2D), {offsetof(GPUParticleData2D, position), offsetof(GPUParticleData2D, color)});
        material_factory.SetTranslucent(false, 1);
        material_factory.SetPointTopology();
        material_factory.CreatePipeline(
            logical_device, render_pass, particle_render_shader.get(), m_particle_render_material.get(), 0);
        m_particle_render_material->SetDebugName("Particle Render Material");
    }

    void ComputeParticlePass::RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                                  const vk::Extent2D&               extent)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // clear

        framebuffers.clear();

        // Create attachment

        vk::ImageView attachments[1];

        // Provide attachment information to frame buffer
        vk::FramebufferCreateInfo framebuffer_create_info(vk::FramebufferCreateFlags(), /* flags */
                                                          *render_pass,                 /* renderPass */
                                                          1,                            /* attachmentCount */
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

    void ComputeParticlePass::UpdateUniformBuffer() {}

    void ComputeParticlePass::Start(const vk::raii::CommandBuffer& command_buffer,
                                    vk::Extent2D                   extent,
                                    uint32_t                       current_image_index)
    {
        draw_call[0] = 0;

        RenderPassBase::Start(command_buffer, extent, current_image_index);

        command_buffer.setViewport(0,
                                   vk::Viewport(0.0f,
                                                static_cast<float>(extent.height),
                                                static_cast<float>(extent.width),
                                                -static_cast<float>(extent.height),
                                                0.0f,
                                                1.0f));
        command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
    }

    void ComputeParticlePass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_particle_render_material->BindPipeline(command_buffer);

        RenderParticles(command_buffer);
    }

    void ComputeParticlePass::RenderParticles(const vk::raii::CommandBuffer& command_buffer) { FUNCTION_TIMER(); }

    void swap(ComputeParticlePass& lhs, ComputeParticlePass& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPassBase&>(lhs), static_cast<RenderPassBase&>(rhs));

        swap(lhs.m_particle_render_material, rhs.m_particle_render_material);

        swap(lhs.draw_call, rhs.draw_call);
    }
} // namespace Meow
