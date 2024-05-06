#pragma once

#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"
#include "render_pass.h"

namespace Meow
{
    class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        ForwardPass(ForwardPass&& rhs) noexcept
            : RenderPass(nullptr)
        {
            std::swap(forward_mat, rhs.forward_mat);
            std::swap(render_pass, rhs.render_pass);
            std::swap(framebuffers, rhs.framebuffers);
            m_depth_format = rhs.m_depth_format;
            m_sample_count = rhs.m_sample_count;
            std::swap(m_depth_attachment, rhs.m_depth_attachment);
        }

        ForwardPass& operator=(ForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::swap(forward_mat, rhs.forward_mat);
                std::swap(render_pass, rhs.render_pass);
                std::swap(framebuffers, rhs.framebuffers);
                m_depth_format = rhs.m_depth_format;
                m_sample_count = rhs.m_sample_count;
                std::swap(m_depth_attachment, rhs.m_depth_attachment);
            }
            return *this;
        }

        ForwardPass(vk::raii::PhysicalDevice const& physical_device,
                    vk::raii::Device const&         device,
                    SurfaceData&                    surface_data,
                    vk::raii::CommandPool const&    command_pool,
                    vk::raii::Queue const&          queue,
                    DescriptorAllocatorGrowable&    m_descriptor_allocator);

        ~ForwardPass()
        {
            forward_mat = nullptr;
            render_pass = nullptr;
            framebuffers.clear();
            m_depth_attachment = nullptr;
        }

        void RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                 vk::raii::Device const&                 device,
                                 vk::raii::CommandBuffer const&          command_buffer,
                                 SurfaceData&                            surface_data,
                                 std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                 vk::Extent2D const&                     extent) override;

        void UpdateUniformBuffer() override;

        void UpdateGUI() override;
        
    public:
        Material forward_mat = nullptr;
    };
} // namespace Meow
