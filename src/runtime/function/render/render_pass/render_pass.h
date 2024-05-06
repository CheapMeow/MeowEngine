#pragma once

#include "core/base/non_copyable.h"
#include "function/render/structs/image_data.h"
#include "function/render/structs/surface_data.h"
#include "function/render/structs/vertex_attribute.h"

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    class RenderPass : public NonCopyable
    {
    public:
        RenderPass(std::nullptr_t) {}

        virtual ~RenderPass() {}

        virtual void RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                         vk::raii::Device const&                 device,
                                         vk::raii::CommandBuffer const&          command_buffer,
                                         SurfaceData&                            surface_data,
                                         std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                         vk::Extent2D const&                     extent)
        {}

        virtual void UpdateUniformBuffer() {}

        virtual void UpdateGUI() {}

        virtual void Draw(vk::raii::CommandBuffer const& command_buffer) {}

        virtual void AfterRenderPass() {}

        vk::raii::RenderPass               render_pass = nullptr;
        std::vector<vk::raii::Framebuffer> framebuffers;
        std::vector<vk::ClearValue>        clear_values;
        std::vector<VertexAttribute>       input_vertex_attributes;

        bool                enable_query = false;
        vk::raii::QueryPool query_pool   = nullptr;

    protected:
        vk::Format              m_depth_format = vk::Format::eD16Unorm;
        vk::SampleCountFlagBits m_sample_count = vk::SampleCountFlagBits::e1;

        std::shared_ptr<ImageData> m_depth_attachment = nullptr;

        enum statisticName
        {
            // Input Assembly
            vertexCount_ia,
            primitiveCount_ia,
            // Vertex Shader
            invocationCount_vs,
            // Geometry Shader
            invocationCount_gs,
            primitiveCount_gs,
            invocationCount_clipping,
            primitiveCount_clipping,
            // Fragment Shader
            invocationCount_fs,
            // Tessellation
            patchCount_tcs,
            invocationCount_tes,
            // Compute Shader
            invocationCount_cs,
            statisticCount
        };
        uint32_t statistics[statisticCount]     = {};
        uint32_t cur_query_count                = 0;
        uint32_t invocationCount_fs_query_count = 0;
    };
} // namespace Meow
