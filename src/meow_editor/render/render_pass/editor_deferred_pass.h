#pragma once

#include "meow_runtime/function/render/render_pass/deferred_pass.h"
#include "render/structs/builtin_render_stat.h"

namespace Meow
{

    class EditorDeferredPass : public DeferredPass
    {
    public:
        EditorDeferredPass(std::nullptr_t)
            : DeferredPass(nullptr)
        {}

        EditorDeferredPass(SurfaceData& surface_data);

        EditorDeferredPass(EditorDeferredPass&& rhs) noexcept
            : DeferredPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        EditorDeferredPass& operator=(EditorDeferredPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                DeferredPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~EditorDeferredPass() override = default;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        void AfterPresent() override;

        friend void swap(EditorDeferredPass& lhs, EditorDeferredPass& rhs);

    private:
        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;

        BuiltinRenderStat m_render_stat[2];
    };
} // namespace Meow
