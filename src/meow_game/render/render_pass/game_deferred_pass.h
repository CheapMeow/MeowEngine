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

        GameDeferredPass(SurfaceData& surface_data);

        GameDeferredPass(GameDeferredPass&& rhs) noexcept
            : DeferredPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        GameDeferredPass& operator=(GameDeferredPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                DeferredPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~GameDeferredPass() override = default;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;
    };
} // namespace Meow
