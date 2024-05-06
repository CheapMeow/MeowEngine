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
    struct AttachmentParamBlock
    {
        float zNear;
        float zFar;
        float padding;
        int   attachmentIndex;
    };

    class DeferredPass : NonCopyable
    {
    public:
        DeferredPass(std::nullptr_t) {}

        DeferredPass(DeferredPass&& rhs) noexcept
        {
            std::swap(obj2attachment_mat, rhs.obj2attachment_mat);
            std::swap(quad_mat, rhs.quad_mat);
            std::swap(quad_model, rhs.quad_model);
            debug_para = rhs.debug_para;
            std::swap(debug_names, rhs.debug_names);
            std::swap(render_pass, rhs.render_pass);
            std::swap(framebuffers, rhs.framebuffers);
            depth_format = rhs.depth_format;
            sample_count = rhs.sample_count;
            std::swap(m_color_attachment, rhs.m_color_attachment);
            std::swap(m_normal_attachment, rhs.m_normal_attachment);
            std::swap(m_depth_attachment, rhs.m_depth_attachment);
        }

        DeferredPass& operator=(DeferredPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::swap(obj2attachment_mat, rhs.obj2attachment_mat);
                std::swap(quad_mat, rhs.quad_mat);
                std::swap(quad_model, rhs.quad_model);
                debug_para = rhs.debug_para;
                std::swap(debug_names, rhs.debug_names);
                std::swap(render_pass, rhs.render_pass);
                std::swap(framebuffers, rhs.framebuffers);
                depth_format = rhs.depth_format;
                sample_count = rhs.sample_count;
                std::swap(m_color_attachment, rhs.m_color_attachment);
                std::swap(m_normal_attachment, rhs.m_normal_attachment);
                std::swap(m_depth_attachment, rhs.m_depth_attachment);
            }
            return *this;
        }

        DeferredPass(vk::raii::PhysicalDevice const& physical_device,
                     vk::raii::Device const&         device,
                     SurfaceData&                    surface_data,
                     vk::raii::CommandPool const&    command_pool,
                     vk::raii::Queue const&          queue,
                     DescriptorAllocatorGrowable&    m_descriptor_allocator);

        ~DeferredPass()
        {
            obj2attachment_mat = nullptr;
            quad_mat           = nullptr;
            quad_model         = nullptr;
            render_pass        = nullptr;
            framebuffers.clear();
            m_color_attachment  = nullptr;
            m_normal_attachment = nullptr;
            m_depth_attachment  = nullptr;
        }

        void RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                 vk::raii::Device const&                 device,
                                 vk::raii::CommandBuffer const&          command_buffer,
                                 SurfaceData&                            surface_data,
                                 std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                 vk::Extent2D const&                     extent);

    public:
        Material obj2attachment_mat = nullptr;
        Material quad_mat           = nullptr;
        Model    quad_model         = nullptr;

        AttachmentParamBlock     debug_para;
        std::vector<const char*> debug_names = {"Color", "Depth", "Normal"};

        vk::raii::RenderPass               render_pass = nullptr;
        std::vector<vk::raii::Framebuffer> framebuffers;

        std::array<vk::ClearValue, 4> clear_values;

    private:
        vk::Format              depth_format = vk::Format::eD16Unorm;
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;

        std::shared_ptr<ImageData> m_color_attachment  = nullptr;
        std::shared_ptr<ImageData> m_normal_attachment = nullptr;
        std::shared_ptr<ImageData> m_depth_attachment  = nullptr;
    };
} // namespace Meow
