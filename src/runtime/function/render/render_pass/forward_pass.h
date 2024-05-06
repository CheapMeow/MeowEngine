#pragma once

#include "core/base/non_copyable.h"
#include "function/render/structs/image_data.h"
#include "function/render/structs/material.h"
#include "function/render/structs/model.h"
#include "function/render/structs/shader.h"
#include "function/render/structs/surface_data.h"

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    class ForwardPass : NonCopyable
    {
    public:
        ForwardPass(std::nullptr_t) {}

        ForwardPass(ForwardPass&& rhs) noexcept
        {
            std::swap(forward_mat, rhs.forward_mat);
            std::swap(render_pass, rhs.render_pass);
            std::swap(framebuffers, rhs.framebuffers);
            depth_format = rhs.depth_format;
            sample_count = rhs.sample_count;
            std::swap(m_depth_attachment, rhs.m_depth_attachment);
        }

        ForwardPass& operator=(ForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::swap(forward_mat, rhs.forward_mat);
                std::swap(render_pass, rhs.render_pass);
                std::swap(framebuffers, rhs.framebuffers);
                depth_format = rhs.depth_format;
                sample_count = rhs.sample_count;
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
                                 vk::Extent2D const&                     extent);

    public:
        Material forward_mat = nullptr;

        vk::raii::RenderPass               render_pass = nullptr;
        std::vector<vk::raii::Framebuffer> framebuffers;

        std::array<vk::ClearValue, 2> clear_values;

    private:
        vk::Format              depth_format = vk::Format::eD16Unorm;
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;

        std::shared_ptr<ImageData> m_depth_attachment = nullptr;
    };
} // namespace Meow
