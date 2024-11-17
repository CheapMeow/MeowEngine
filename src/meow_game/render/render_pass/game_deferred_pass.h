#pragma once

#include "meow_runtime/function/render/render_pass/deferred_pass.h"

namespace Meow
{
    class GameDeferredPass : public DeferredPass
    {
    public:
        GameDeferredPass(std::nullptr_t)
            : DeferredPass(nullptr)
        {}

        GameDeferredPass(const vk::raii::PhysicalDevice& physical_device,
                         const vk::raii::Device&         logical_device,
                         SurfaceData&                    surface_data,
                         const vk::raii::CommandPool&    command_pool,
                         const vk::raii::Queue&          queue,
                         DescriptorAllocatorGrowable&    m_descriptor_allocator);

        GameDeferredPass(GameDeferredPass&& rhs) noexcept
            : DeferredPass(std::move(rhs))
        {}

        GameDeferredPass& operator=(GameDeferredPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                DeferredPass::operator=(std::move(rhs));
            }
            return *this;
        }

        ~GameDeferredPass() override = default;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;
    };
} // namespace Meow