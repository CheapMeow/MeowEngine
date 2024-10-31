#pragma once

#include "function/render/structs/builtin_render_stat.h"
#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"
#include "meow_runtime/function/render/render_pass/render_pass.h"

namespace Meow
{
    class GameForwardPass : public RenderPass
    {
    public:
        GameForwardPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        GameForwardPass(const vk::raii::PhysicalDevice& physical_device,
                        const vk::raii::Device&         device,
                        SurfaceData&                    surface_data,
                        const vk::raii::CommandPool&    command_pool,
                        const vk::raii::Queue&          queue,
                        DescriptorAllocatorGrowable&    m_descriptor_allocator);

        GameForwardPass(GameForwardPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        GameForwardPass& operator=(GameForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~GameForwardPass() override
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

        friend void swap(GameForwardPass& lhs, GameForwardPass& rhs);

    private:
        Material m_forward_mat = nullptr;

        int draw_call = 0;
    };
} // namespace Meow