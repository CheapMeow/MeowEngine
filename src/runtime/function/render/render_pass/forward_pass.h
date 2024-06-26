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
            std::swap(m_forward_mat, rhs.m_forward_mat);
            std::swap(render_pass, rhs.render_pass);
            std::swap(framebuffers, rhs.framebuffers);
            std::swap(clear_values, rhs.clear_values);
            std::swap(input_vertex_attributes, rhs.input_vertex_attributes);
            m_depth_format = rhs.m_depth_format;
            m_sample_count = rhs.m_sample_count;
            std::swap(m_depth_attachment, rhs.m_depth_attachment);
            std::swap(query_pool, rhs.query_pool);
            enable_query = rhs.enable_query;
        }

        ForwardPass& operator=(ForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::swap(m_forward_mat, rhs.m_forward_mat);
                std::swap(render_pass, rhs.render_pass);
                std::swap(framebuffers, rhs.framebuffers);
                std::swap(clear_values, rhs.clear_values);
                std::swap(input_vertex_attributes, rhs.input_vertex_attributes);
                m_depth_format = rhs.m_depth_format;
                m_sample_count = rhs.m_sample_count;
                std::swap(m_depth_attachment, rhs.m_depth_attachment);
                std::swap(query_pool, rhs.query_pool);
                enable_query = rhs.enable_query;
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
            m_forward_mat = nullptr;
            render_pass   = nullptr;
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

        void Draw(vk::raii::CommandBuffer const& command_buffer) override;

        void AfterRenderPass() override;

    private:
        Material m_forward_mat = nullptr;
    };
} // namespace Meow
