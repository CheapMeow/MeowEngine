#pragma once

#include "core/base/non_copyable.h"
#include "function/render/buffer_data/image_data.h"
#include "function/render/buffer_data/surface_data.h"
#include "function/render/model/vertex_attribute.h"

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    class RenderPassBase : public NonCopyable
    {
    public:
        RenderPassBase(std::nullptr_t) {}

        RenderPassBase() {}

        RenderPassBase(SurfaceData& surface_data);

        RenderPassBase(RenderPassBase&& rhs) noexcept { swap(*this, rhs); }

        RenderPassBase& operator=(RenderPassBase&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~RenderPassBase() override = default;

        virtual void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                         const vk::Extent2D&               extent)
        {}

        virtual void UpdateUniformBuffer() {}

        virtual void
        Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t current_image_index);

        virtual void Draw(const vk::raii::CommandBuffer& command_buffer) {}

        virtual void End(const vk::raii::CommandBuffer& command_buffer);

        virtual void AfterPresent();

        friend void swap(RenderPassBase& lhs, RenderPassBase& rhs);

        vk::raii::RenderPass               render_pass = nullptr;
        std::vector<vk::raii::Framebuffer> framebuffers;
        std::vector<vk::ClearValue>        clear_values;
        std::vector<VertexAttributeBit>    input_vertex_attributes;

    protected:
        vk::Format m_color_format;
        vk::Format m_depth_format = vk::Format::eD16Unorm;
    };
} // namespace Meow