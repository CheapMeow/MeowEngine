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

        RenderPass(const vk::raii::Device& device);

        RenderPass(RenderPass&& rhs) noexcept { swap(*this, rhs); }

        RenderPass& operator=(RenderPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~RenderPass() override {}

        virtual void RefreshFrameBuffers(const vk::raii::PhysicalDevice&   physical_device,
                                         const vk::raii::Device&           device,
                                         const vk::raii::CommandPool&      command_pool,
                                         const vk::raii::Queue&            queue,
                                         const std::vector<vk::ImageView>& output_image_views,
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
        BitMask<VertexAttributeBit>        input_vertex_attributes;

        std::string m_pass_name = "Default Pass";

    protected:
        vk::Format                 m_depth_format     = vk::Format::eD16Unorm;
        vk::SampleCountFlagBits    m_sample_count     = vk::SampleCountFlagBits::e1;
        std::shared_ptr<ImageData> m_depth_attachment = nullptr;
    };
} // namespace Meow