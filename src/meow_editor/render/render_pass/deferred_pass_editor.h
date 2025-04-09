#pragma once

#include "meow_runtime/function/render/render_pass/deferred_pass_base.h"
#include "render/structs/builtin_render_stat.h"

namespace Meow
{

    class DeferredPassEditor : public DeferredPassBase
    {
    public:
        DeferredPassEditor(std::nullptr_t)
            : DeferredPassBase(nullptr)
        {}

        DeferredPassEditor(SurfaceData& surface_data);

        DeferredPassEditor(DeferredPassEditor&& rhs) noexcept
            : DeferredPassBase(nullptr)
        {
            swap(*this, rhs);
        }

        DeferredPassEditor& operator=(DeferredPassEditor&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~DeferredPassEditor() override = default;

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) override;

        void AfterPresent() override;

        friend void swap(DeferredPassEditor& lhs, DeferredPassEditor& rhs);

    private:
        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;

        BuiltinRenderStat m_render_stat[2];
    };
} // namespace Meow
