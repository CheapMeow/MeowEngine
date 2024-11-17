#pragma once

#include "meow_runtime/function/render/render_pass/forward_pass.h"

namespace Meow
{
    class GameForwardPass : public ForwardPass
    {
    public:
        GameForwardPass(std::nullptr_t)
            : ForwardPass(nullptr)
        {}

        GameForwardPass(const vk::raii::PhysicalDevice& physical_device,
                        const vk::raii::Device&         logical_device,
                        SurfaceData&                    surface_data,
                        const vk::raii::CommandPool&    command_pool,
                        const vk::raii::Queue&          queue,
                        DescriptorAllocatorGrowable&    m_descriptor_allocator);

        GameForwardPass(GameForwardPass&& rhs) noexcept
            : ForwardPass(std::move(rhs))
        {}

        GameForwardPass& operator=(GameForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                ForwardPass::operator=(std::move(rhs));
            }
            return *this;
        }

        ~GameForwardPass() override = default;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;
    };
} // namespace Meow