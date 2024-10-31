#pragma once

#include "function/render/structs/builtin_render_stat.h"
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

        ForwardPass(const vk::raii::PhysicalDevice& physical_device,
                    const vk::raii::Device&         device,
                    SurfaceData&                    surface_data,
                    const vk::raii::CommandPool&    command_pool,
                    const vk::raii::Queue&          queue,
                    DescriptorAllocatorGrowable&    m_descriptor_allocator);

        ForwardPass(ForwardPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        ForwardPass& operator=(ForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~ForwardPass() override
        {
            m_forward_mat = nullptr;
            render_pass   = nullptr;
            framebuffers.clear();
            m_depth_attachment = nullptr;
        }

        void RefreshFrameBuffers(const vk::raii::PhysicalDevice&         physical_device,
                                 const vk::raii::Device&                 device,
                                 const vk::raii::CommandPool&            command_pool,
                                 const vk::raii::Queue&                  queue,
                                 SurfaceData&                            surface_data,
                                 const std::vector<vk::raii::ImageView>& swapchain_image_views,
                                 const vk::Extent2D&                     extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   const Meow::SurfaceData&       surface_data,
                   uint32_t                       current_image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        void AfterPresent() override;

        friend void swap(ForwardPass& lhs, ForwardPass& rhs);

    private:
        Material m_forward_mat = nullptr;

        int draw_call = 0;

        BuiltinRenderStat m_render_stat;
    };
} // namespace Meow
