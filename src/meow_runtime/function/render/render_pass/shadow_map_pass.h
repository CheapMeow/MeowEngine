#pragma once

#include "function/render/material/material.h"
#include "function/render/material/shader.h"
#include "function/render/model/model.hpp"
#include "function/render/render_pass/render_pass_base.h"
#include "function/render/utils/vulkan_debug_utils.h"

namespace Meow
{
    class ShadowMapPass : public RenderPassBase
    {
    public:
        ShadowMapPass(std::nullptr_t)
            : RenderPassBase(nullptr)
        {}

        ShadowMapPass()
            : RenderPassBase()
        {}

        ShadowMapPass(SurfaceData& surface_data);

        ShadowMapPass(ShadowMapPass&& rhs) noexcept
            : RenderPassBase(nullptr)
        {
            swap(*this, rhs);
        }

        ShadowMapPass& operator=(ShadowMapPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~ShadowMapPass() override = default;

        void CreateRenderPass();
        void CreateMaterial();

        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void RecordGraphicsCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) override;

        void RenderShadowMap(const vk::raii::CommandBuffer& command_buffer);

        std::shared_ptr<ImageData> GetShadowMap() { return m_shadow_map; }

        friend void swap(ShadowMapPass& lhs, ShadowMapPass& rhs);

    protected:
        std::shared_ptr<Material>  m_shadow_map_material = nullptr;
        std::shared_ptr<ImageData> m_shadow_map          = nullptr;

        std::string m_pass_names[1];
        int         draw_call[1] = {0};
    };
} // namespace Meow