#pragma once

#include "function/render/material/material.h"
#include "function/render/material/shader.h"
#include "function/render/model/model.hpp"
#include "function/render/render_pass/render_pass.h"
#include "function/render/utils/vulkan_debug_utils.h"

namespace Meow
{
    class ShadowMapPass : public RenderPass
    {
    public:
        ShadowMapPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        ShadowMapPass()
            : RenderPass()
        {}

        ShadowMapPass(SurfaceData& surface_data)
            : RenderPass(surface_data)
        {}

        ShadowMapPass(ShadowMapPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        ShadowMapPass& operator=(ShadowMapPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~ShadowMapPass() override = default;

        virtual void CreateRenderPass() {}
        void         CreateMaterial();

        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void RenderShadowMap(const vk::raii::CommandBuffer& command_buffer);

        friend void swap(ShadowMapPass& lhs, ShadowMapPass& rhs);

    protected:
        Material                   m_shadow_map_mat = nullptr;
        std::shared_ptr<ImageData> m_shadow_map     = nullptr;

        int draw_call = 0;
    };
} // namespace Meow