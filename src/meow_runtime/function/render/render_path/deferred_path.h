#pragma once

#include "render_path.h"

#include "function/render/render_pass/deferred_lighting_pass.h"
#include "function/render/render_pass/gbuffer_pass.h"
#include "function/render/render_pass/imgui_pass.h"

#include <memory>

namespace Meow
{
    class DeferredPath : public RenderPath
    {
    public:
        DeferredPath(std::nullptr_t)
            : RenderPath(nullptr)
        {}

        DeferredPath(DeferredPath&& rhs) noexcept
            : RenderPath(nullptr)
        {
            swap(*this, rhs);
        }

        DeferredPath& operator=(DeferredPath&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        DeferredPath()
            : RenderPath()
        {
            m_gbuffer_pass        = std::move(GbufferPass());
            m_deferred_light_pass = std::move(DeferredLightingPass());
        }

        ~DeferredPath() override {};

        void RefreshAttachments(vk::Format color_format, const vk::Extent2D& extent) override;

        void UpdateUniformBuffer() override;

        void
        Draw(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, ImageData& swapchain_image) override;

        void AfterPresent();

        friend void swap(DeferredPath& lhs, DeferredPath& rhs);

    private:
        GbufferPass          m_gbuffer_pass        = nullptr;
        DeferredLightingPass m_deferred_light_pass = nullptr;

        ImageData m_color_attachment    = nullptr;
        ImageData m_normal_attachment   = nullptr;
        ImageData m_position_attachment = nullptr;
    };
} // namespace Meow