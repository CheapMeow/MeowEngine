#pragma once

#include "core/base/non_copyable.h"
#include "function/render/render_resources/image_data.h"
#include "function/render/render_resources/vertex_attribute.h"
#include "function/render/structs/surface_data.h"

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    class RenderPass : public NonCopyable
    {
    public:
        RenderPass(std::nullptr_t) {}

        RenderPass(RenderPass&& rhs) noexcept { swap(*this, rhs); }

        RenderPass& operator=(RenderPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        RenderPass() {}

        ~RenderPass() override = default;

        virtual void UpdateUniformBuffer() {}

        virtual void BeforeRender(const vk::raii::CommandBuffer& command_buffer) {};

        virtual void Draw(const vk::raii::CommandBuffer& command_buffer) {}

        virtual void AfterPresent() {}

        friend void swap(RenderPass& lhs, RenderPass& rhs);

    protected:
#ifdef MEOW_EDITOR
        std::string         m_pass_name  = "Default Pass";
        vk::raii::QueryPool m_query_pool = nullptr;
#endif
    };
} // namespace Meow