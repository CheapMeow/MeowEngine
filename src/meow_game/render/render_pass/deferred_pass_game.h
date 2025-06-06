#pragma once

#include "meow_runtime/function/render/render_pass/deferred_pass_base.h"

namespace Meow
{
    class DeferredPassGame : public DeferredPassBase
    {
    public:
        DeferredPassGame(std::nullptr_t)
            : DeferredPassBase(nullptr)
        {}

        DeferredPassGame(SurfaceData& surface_data);

        DeferredPassGame(DeferredPassGame&& rhs) noexcept
            : DeferredPassBase(nullptr)
        {
            swap(*this, rhs);
        }

        DeferredPassGame& operator=(DeferredPassGame&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~DeferredPassGame() override = default;

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void RecordGraphicsCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) override;
    };
} // namespace Meow
