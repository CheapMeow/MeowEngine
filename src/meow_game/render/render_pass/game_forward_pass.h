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

        GameForwardPass(SurfaceData& surface_data);

        GameForwardPass(GameForwardPass&& rhs) noexcept
            : ForwardPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        GameForwardPass& operator=(GameForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                ForwardPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~GameForwardPass() override = default;

        void CreateRenderPass() override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;
    };
} // namespace Meow
