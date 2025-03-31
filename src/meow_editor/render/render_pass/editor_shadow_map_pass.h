#pragma once

#include "meow_runtime/function/render/render_pass/shadow_map_pass.h"
#ifdef MEOW_EDITOR
#    include "render/structs/builtin_render_stat.h"
#endif

namespace Meow
{
    class EditorShadowMapPass : public ShadowMapPass
    {
    public:
        EditorShadowMapPass(std::nullptr_t)
            : ShadowMapPass(nullptr)
        {}

        EditorShadowMapPass(SurfaceData& surface_data);

        EditorShadowMapPass(EditorShadowMapPass&& rhs) noexcept
            : ShadowMapPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        EditorShadowMapPass& operator=(EditorShadowMapPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                ShadowMapPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~EditorShadowMapPass() override = default;

        void CreateRenderPass() override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        void AfterPresent() override;

        friend void swap(EditorShadowMapPass& lhs, EditorShadowMapPass& rhs);

    private:
#ifdef MEOW_EDITOR
        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;

        BuiltinRenderStat m_render_stat[1];
#endif
    };
} // namespace Meow
