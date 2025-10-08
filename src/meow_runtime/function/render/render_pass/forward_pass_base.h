#pragma once

#include "function/render/material/material.h"
#include "function/render/material/shader.h"
#include "function/render/model/model.hpp"
#include "function/render/render_pass/render_pass_base.h"
#include "function/render/utils/vulkan_debug_utils.h"

namespace Meow
{
    class ForwardPassBase : public RenderPassBase
    {
    public:
        ForwardPassBase(std::nullptr_t)
            : RenderPassBase(nullptr)
        {}

        ForwardPassBase()
            : RenderPassBase()
        {}

        ForwardPassBase(SurfaceData& surface_data)
            : RenderPassBase(surface_data)
        {}

        ForwardPassBase(ForwardPassBase&& rhs) noexcept
            : RenderPassBase(nullptr)
        {
            swap(*this, rhs);
        }

        ForwardPassBase& operator=(ForwardPassBase&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~ForwardPassBase() override = default;

        virtual void CreateRenderPass() {}
        void         CreateMaterial();

        void PopulateDirectionalLightData(std::shared_ptr<ImageData> shadow_map, uint32_t frame_index);
        void BindShadowMap(std::shared_ptr<ImageData> shadow_map);

        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer(uint32_t frame_index) override;

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void RenderOpaqueMeshes(const vk::raii::CommandBuffer& command_buffer);

        void RenderSkybox(const vk::raii::CommandBuffer& command_buffer);

        void RenderTranslucentMeshes(const vk::raii::CommandBuffer& command_buffer);

        void SetMSAAEnabled(bool enabled);

        UUID GetForwardMatID() { return m_opaque_material->uuid(); }
        UUID GetTranslucentMatID() { return m_translucent_material->uuid(); }

        friend void swap(ForwardPassBase& lhs, ForwardPassBase& rhs);

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