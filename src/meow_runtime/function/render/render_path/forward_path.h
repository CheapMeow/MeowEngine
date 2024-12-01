#pragma once

#include "render_path.h"

#include "function/render/render_pass/forward_lighting_pass.h"

#include <memory>

namespace Meow
{
    class ForwardPath : public RenderPath
    {
    public:
        ForwardPath(std::nullptr_t)
            : RenderPath(nullptr)
        {}

        ForwardPath(ForwardPath&& rhs) noexcept
            : RenderPath(std::move(rhs))
        {
            swap(*this, rhs);
        }

        ForwardPath& operator=(ForwardPath&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPath::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ForwardPath();

        ~ForwardPath() override {};

        void RefreshAttachments(vk::Format color_format, const vk::Extent2D& extent) override;

        void UpdateUniformBuffer() override;

        void
        Draw(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, ImageData& swapchain_image) override;

        void AfterPresent();

        friend void swap(ForwardPath& lhs, ForwardPath& rhs);

    private:
        ForwardLightingPass m_forward_light_pass = nullptr;
    };
} // namespace Meow