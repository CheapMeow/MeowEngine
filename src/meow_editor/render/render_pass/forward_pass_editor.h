#pragma once

#include "meow_runtime/function/render/render_pass/forward_pass_base.h"
#include "render/structs/builtin_render_stat.h"

namespace Meow
{
    class ForwardPassEditor : public ForwardPassBase
    {
    public:
        ForwardPassEditor(std::nullptr_t)
            : ForwardPassBase(nullptr)
        {}

        ForwardPassEditor(SurfaceData& surface_data);

        ForwardPassEditor(ForwardPassEditor&& rhs) noexcept
            : ForwardPassBase(nullptr)
        {
            swap(*this, rhs);
        }

        ForwardPassEditor& operator=(ForwardPassEditor&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~ForwardPassEditor() override = default;

        void CreateRenderPass() override;

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void RecordGraphicsCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) override;

        void AfterPresent() override;

        friend void swap(ForwardPassEditor& lhs, ForwardPassEditor& rhs);

    private:
        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;

        BuiltinRenderStat m_render_stat[2];
    };
} // namespace Meow
