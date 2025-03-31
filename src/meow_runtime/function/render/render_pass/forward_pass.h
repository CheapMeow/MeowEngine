#pragma once

#include "function/render/material/material.h"
#include "function/render/material/shader.h"
#include "function/render/model/model.hpp"
#include "function/render/render_pass/render_pass.h"
#include "function/render/utils/vulkan_debug_utils.h"

namespace Meow
{
    class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        ForwardPass()
            : RenderPass()
        {}

        ForwardPass(SurfaceData& surface_data)
            : RenderPass(surface_data)
        {}

        ForwardPass(ForwardPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        ForwardPass& operator=(ForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~ForwardPass() override = default;

        virtual void CreateRenderPass() {}
        void         CreateMaterial();

        void PopulateDirectionalLightData(std::shared_ptr<ImageData> shadow_map);
        void BindShadowMap(std::shared_ptr<ImageData> shadow_map);

        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void RenderOpaqueMeshes(const vk::raii::CommandBuffer& command_buffer);

        void RenderSkybox(const vk::raii::CommandBuffer& command_buffer);

        void RenderTranslucentMeshes(const vk::raii::CommandBuffer& command_buffer);

        void SetMSAAEnabled(bool enabled);

        UUID GetForwardMatID() { return m_opaque_material->uuid(); }
        UUID GetTranslucentMatID() { return m_translucent_material->uuid(); }

        friend void swap(ForwardPass& lhs, ForwardPass& rhs);

    protected:
        std::shared_ptr<Material> m_opaque_material = nullptr;

        std::shared_ptr<Material> m_skybox_material = nullptr;
        Model                     m_skybox_model    = nullptr;

        std::shared_ptr<Material> m_translucent_material = nullptr;

        bool                       m_msaa_enabled          = true;
        std::shared_ptr<ImageData> m_color_msaa_attachment = nullptr;

        std::shared_ptr<ImageData> m_depth_attachment = nullptr;

        std::string m_pass_names[2];
        int         draw_call[3] = {0, 0, 0};
    };
} // namespace Meow