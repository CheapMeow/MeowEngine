#pragma once

#include "core/base/non_copyable.h"
#include "function/render/render_resources/image_data.h"
#include "function/render/render_resources/surface_data.h"
#include "function/render/render_resources/vertex_attribute.h"


#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    class RenderPass : public NonCopyable
    {
    public:
        RenderPass(std::nullptr_t) {}

        RenderPass() {}

        RenderPass(RenderPass&& rhs) noexcept { swap(*this, rhs); }

        RenderPass& operator=(RenderPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~RenderPass() override = default;

        virtual void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                         const vk::Extent2D&               extent)
        {}

        virtual void UpdateUniformBuffer() {}

        virtual void
        Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t current_image_index);

        virtual void Draw(const vk::raii::CommandBuffer& command_buffer) {}

        virtual void End(const vk::raii::CommandBuffer& command_buffer);

        virtual void AfterPresent();

        friend void swap(RenderPass& lhs, RenderPass& rhs);

        vk::raii::RenderPass               render_pass = nullptr;
        std::vector<vk::raii::Framebuffer> framebuffers;
        std::vector<vk::ClearValue>        clear_values;
        std::vector<VertexAttributeBit>    input_vertex_attributes;

    protected:
        vk::Format                 m_depth_format     = vk::Format::eD16Unorm;
        vk::SampleCountFlagBits    m_sample_count     = vk::SampleCountFlagBits::e1;
        std::shared_ptr<ImageData> m_depth_attachment = nullptr;
    };
} // namespace Meow