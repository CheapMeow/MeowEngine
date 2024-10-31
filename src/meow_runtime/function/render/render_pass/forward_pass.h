#pragma once

#include "core/base/macro.h"
#include "function/render/structs/builtin_render_stat.h"
#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"
#include "render_pass.h"

namespace Meow
{
    class LIBRARY_API ForwardPass : public RenderPass
    {
    public:
        ForwardPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        ForwardPass(vk::raii::PhysicalDevice const& physical_device,
                    vk::raii::Device const&         device,
                    SurfaceData&                    surface_data,
                    vk::raii::CommandPool const&    command_pool,
                    vk::raii::Queue const&          queue,
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

        ~ForwardPass()
        {
            m_forward_mat = nullptr;
            render_pass   = nullptr;
            framebuffers.clear();
            m_depth_attachment = nullptr;
        }

        void RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                 vk::raii::Device const&                 device,
                                 vk::raii::CommandPool const&            command_pool,
                                 vk::raii::Queue const&                  queue,
                                 SurfaceData&                            surface_data,
                                 std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                 vk::Extent2D const&                     extent) override;

        void UpdateUniformBuffer() override;

        void Start(vk::raii::CommandBuffer const& command_buffer,
                   Meow::SurfaceData const&       surface_data,
                   uint32_t                       current_image_index) override;

        void Draw(vk::raii::CommandBuffer const& command_buffer) override;

        void AfterPresent() override;

        friend void LIBRARY_API swap(ForwardPass& lhs, ForwardPass& rhs);

    private:
        Material m_forward_mat = nullptr;

        BuiltinRenderStat m_render_stat;
    };
} // namespace Meow