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
#ifdef MEOW_EDITOR
        void CreateMaterial();
#endif

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

#ifdef MEOW_EDITOR
        void RenderDepthToColor(const vk::raii::CommandBuffer& command_buffer);
#endif

        void AfterPresent() override;

        std::shared_ptr<ImageData> GetDepthToColorAttachment() {return m_depth_to_color_attachment;}
        
        friend void swap(EditorShadowMapPass& lhs, EditorShadowMapPass& rhs);

    private:
#ifdef MEOW_EDITOR
        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;

        BuiltinRenderStat m_render_stat[1];

        std::shared_ptr<Material>  m_depth_to_color_material   = nullptr;
        std::shared_ptr<ImageData> m_depth_to_color_attachment = nullptr;
        Model                      m_quad_model                = nullptr;
#endif
    };
} // namespace Meow
