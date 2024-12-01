#pragma once

#include "core/base/non_copyable.h"

#include "function/render/render_pass/imgui_pass.h"
#include "function/render/render_resources/image_data.h"

namespace Meow
{
    class RenderPath : public NonCopyable
    {
    public:
        RenderPath(std::nullptr_t) {}

        RenderPath(RenderPath&& rhs) noexcept { swap(*this, rhs); }

        RenderPath& operator=(RenderPath&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        RenderPath()
        {
#ifdef MEOW_EDITOR
            m_imgui_pass = std::move(ImGuiPass());
#endif
        }

        virtual ~RenderPath() override {};

        virtual void RefreshAttachments(vk::Format color_format, const vk::Extent2D& extent) {}

        virtual void UpdateUniformBuffer() {}

        virtual void Draw(const vk::raii::CommandBuffer& command_buffer,
                          vk::Extent2D                   extent,
                          const vk::Image&               swapchain_image,
                          const vk::raii::ImageView&     swapchain_image_view)
        {}

        virtual void AfterPresent() {}

#ifdef MEOW_EDITOR
        ImGuiPass& GetImGuiPass() { return m_imgui_pass; }
#endif

        friend void swap(RenderPath& lhs, RenderPath& rhs);

    protected:
        vk::Format m_depth_format     = vk::Format::eD16Unorm;
        ImageData  m_depth_attachment = nullptr;

#ifdef MEOW_EDITOR
        ImGuiPass m_imgui_pass = nullptr;

        ImageData m_offscreen_render_target = nullptr;
#endif
    };
} // namespace Meow
