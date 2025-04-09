#pragma once

#include "function/render/material/material.h"
#include "function/render/model/model.hpp"
#include "meow_runtime/function/render/render_pass/render_pass_base.h"
#include "render/structs/builtin_render_stat.h"

namespace Meow
{
    class ShadowCoordToColorPass : public RenderPassBase
    {
    public:
        ShadowCoordToColorPass(std::nullptr_t)
            : RenderPassBase(nullptr)
        {}

        ShadowCoordToColorPass(SurfaceData& surface_data);

        ShadowCoordToColorPass(ShadowCoordToColorPass&& rhs) noexcept
            : RenderPassBase(nullptr)
        {
            swap(*this, rhs);
        }

        ShadowCoordToColorPass& operator=(ShadowCoordToColorPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~ShadowCoordToColorPass() override = default;

        void CreateRenderPass();
        void CreateMaterial();

        void BindDepthAttachment(std::shared_ptr<ImageData> depth_attachment)
        {
            m_depth_debugging_attachment = depth_attachment;
        }
        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        std::shared_ptr<ImageData> GetShadowCoordToColorRenderTarget() { return m_shadow_coord_to_color_render_target; }
        std::shared_ptr<ImageData> GetShadowDepthToColorRenderTarget() { return m_shadow_depth_to_color_render_target; }

        friend void swap(ShadowCoordToColorPass& lhs, ShadowCoordToColorPass& rhs);

    private:
        std::shared_ptr<ImageData> m_depth_debugging_attachment = nullptr;

        std::shared_ptr<Material>  m_shadow_coord_to_color_material      = nullptr;
        std::shared_ptr<ImageData> m_shadow_coord_to_color_render_target = nullptr;
        std::shared_ptr<ImageData> m_shadow_depth_to_color_render_target = nullptr;
        Model                      m_quad_model                          = nullptr;

        int draw_call[1] = {0};
    };
} // namespace Meow
