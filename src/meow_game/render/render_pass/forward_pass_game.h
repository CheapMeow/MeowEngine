#pragma once

#include "meow_runtime/function/render/render_pass/forward_pass_base.h"

namespace Meow
{
    class ForwardPassGame : public ForwardPassBase
    {
    public:
        ForwardPassGame(std::nullptr_t)
            : ForwardPassBase(nullptr)
        {}

        ForwardPassGame(SurfaceData& surface_data);

        ForwardPassGame(ForwardPassGame&& rhs) noexcept
            : ForwardPassBase(nullptr)
        {
            swap(*this, rhs);
        }

        ForwardPassGame& operator=(ForwardPassGame&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~ForwardPassGame() override = default;

        void CreateRenderPass() override;

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void RecordGraphicsCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) override;
    };
} // namespace Meow
