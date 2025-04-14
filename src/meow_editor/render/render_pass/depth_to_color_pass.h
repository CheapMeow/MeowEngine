#pragma once

#include "function/render/material/material.h"
#include "function/render/model/model.hpp"
#include "meow_runtime/function/render/render_pass/render_pass_base.h"
#include "render/structs/builtin_render_stat.h"

namespace Meow
{
    class DepthToColorPass : public RenderPassBase
    {
    public:
        DepthToColorPass(std::nullptr_t)
            : RenderPassBase(nullptr)
        {}

        DepthToColorPass(SurfaceData& surface_data);

        DepthToColorPass(DepthToColorPass&& rhs) noexcept
            : RenderPassBase(nullptr)
        {
            swap(*this, rhs);
        }

        DepthToColorPass& operator=(DepthToColorPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~DepthToColorPass() override = default;

        void CreateRenderPass();
        void CreateMaterial();

        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void RecordGraphicsCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) override;

        void                       BindShadowMap(std::shared_ptr<ImageData> shadow_map);
        std::shared_ptr<ImageData> GetDepthToColorRenderTarget() { return m_depth_to_color_render_target; }

        friend void swap(DepthToColorPass& lhs, DepthToColorPass& rhs);

    private:
        std::shared_ptr<Material>  m_depth_to_color_material      = nullptr;
        std::shared_ptr<ImageData> m_depth_to_color_render_target = nullptr;
        Model                      m_quad_model                   = nullptr;
    };
} // namespace Meow
