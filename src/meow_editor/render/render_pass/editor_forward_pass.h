#pragma once

#include "meow_runtime/function/render/render_pass/forward_pass.h"
#ifdef MEOW_EDITOR
#    include "render/structs/builtin_render_stat.h"
#endif

namespace Meow
{
    class EditorForwardPass : public ForwardPass
    {
    public:
        EditorForwardPass(std::nullptr_t)
            : ForwardPass(nullptr)
        {}

        EditorForwardPass(SurfaceData& surface_data);

        EditorForwardPass(EditorForwardPass&& rhs) noexcept
            : ForwardPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        EditorForwardPass& operator=(EditorForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                ForwardPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~EditorForwardPass() override = default;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        void AfterPresent() override;

        friend void swap(EditorForwardPass& lhs, EditorForwardPass& rhs);

    private:
        void CreateRenderPass();

#ifdef MEOW_EDITOR
        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;

        BuiltinRenderStat m_render_stat[2];
#endif
    };
} // namespace Meow
