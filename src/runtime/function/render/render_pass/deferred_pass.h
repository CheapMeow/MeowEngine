#pragma once

#include "function/render/structs/material.h"
#include "function/render/structs/model.h"
#include "function/render/structs/shader.h"
#include "render_pass.h"

namespace Meow
{
    struct AttachmentParamBlock
    {
        float zNear           = 300.0f;
        float zFar            = 3000.0f;
        float padding         = 0.0f;
        int   attachmentIndex = 0;
    };

    class DeferredPass : public RenderPass
    {
    public:
        DeferredPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        DeferredPass(DeferredPass&& rhs) noexcept
            : RenderPass(nullptr)
        {
            std::swap(m_obj2attachment_mat, rhs.m_obj2attachment_mat);
            std::swap(m_quad_mat, rhs.m_quad_mat);
            std::swap(m_quad_model, rhs.m_quad_model);
            debug_para = rhs.debug_para;
            std::swap(debug_names, rhs.debug_names);
            std::swap(render_pass, rhs.render_pass);
            std::swap(framebuffers, rhs.framebuffers);
            std::swap(clear_values, rhs.clear_values);
            std::swap(input_vertex_attributes, rhs.input_vertex_attributes);
            m_depth_format = rhs.m_depth_format;
            m_sample_count = rhs.m_sample_count;
            std::swap(m_color_attachment, rhs.m_color_attachment);
            std::swap(m_normal_attachment, rhs.m_normal_attachment);
            std::swap(m_depth_attachment, rhs.m_depth_attachment);
            std::swap(query_pool, rhs.query_pool);
            enable_query = rhs.enable_query;
        }

        DeferredPass& operator=(DeferredPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::swap(m_obj2attachment_mat, rhs.m_obj2attachment_mat);
                std::swap(m_quad_mat, rhs.m_quad_mat);
                std::swap(m_quad_model, rhs.m_quad_model);
                debug_para = rhs.debug_para;
                std::swap(debug_names, rhs.debug_names);
                std::swap(render_pass, rhs.render_pass);
                std::swap(framebuffers, rhs.framebuffers);
                std::swap(clear_values, rhs.clear_values);
                std::swap(input_vertex_attributes, rhs.input_vertex_attributes);
                m_depth_format = rhs.m_depth_format;
                m_sample_count = rhs.m_sample_count;
                std::swap(m_color_attachment, rhs.m_color_attachment);
                std::swap(m_normal_attachment, rhs.m_normal_attachment);
                std::swap(m_depth_attachment, rhs.m_depth_attachment);
                std::swap(query_pool, rhs.query_pool);
                enable_query = rhs.enable_query;
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
            m_obj2attachment_mat = nullptr;
            m_quad_mat           = nullptr;
            m_quad_model         = nullptr;
            render_pass          = nullptr;
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
                                 vk::Extent2D const&                     extent) override;

        void UpdateUniformBuffer() override;

        void UpdateGUI() override;

        void Draw(vk::raii::CommandBuffer const& command_buffer) override;

        void AfterRenderPass() override;

    private:
        Material m_obj2attachment_mat = nullptr;
        Material m_quad_mat           = nullptr;
        Model    m_quad_model         = nullptr;

        std::shared_ptr<ImageData> m_color_attachment  = nullptr;
        std::shared_ptr<ImageData> m_normal_attachment = nullptr;

        AttachmentParamBlock     debug_para;
        std::vector<const char*> debug_names = {"Color", "Depth", "Normal"};
    };
} // namespace Meow
