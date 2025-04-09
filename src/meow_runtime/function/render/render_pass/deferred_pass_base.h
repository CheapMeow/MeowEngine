#pragma once

#include "function/render/material/material.h"
#include "function/render/material/shader.h"
#include "function/render/model/model.hpp"
#include "function/render/render_pass/render_pass_base.h"

namespace Meow
{
    constexpr int k_num_lights = 64;

    struct PointLight
    {
        glm::vec4 position;
        glm::vec3 color;
        float     radius;
    };

    struct LightSpawnBlock
    {
        glm::vec3 position[k_num_lights];
        glm::vec3 direction[k_num_lights];
        float     speed[k_num_lights];
    };

    struct LightDataBlock
    {
        PointLight lights[k_num_lights];
    };

    class DeferredPassBase : public RenderPassBase
    {
    public:
        DeferredPassBase(std::nullptr_t)
            : RenderPassBase(nullptr)
        {}

        DeferredPassBase()
            : RenderPassBase()
        {}

        DeferredPassBase(SurfaceData& surface_data)
            : RenderPassBase(surface_data)
        {}

        DeferredPassBase(DeferredPassBase&& rhs) noexcept
            : RenderPassBase(nullptr)
        {
            swap(*this, rhs);
        }

        DeferredPassBase& operator=(DeferredPassBase&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~DeferredPassBase() override = default;

        void CreateMaterial();

        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void RenderGBuffer(const vk::raii::CommandBuffer& command_buffer);

        void RenderOpaqueMeshes(const vk::raii::CommandBuffer& command_buffer);

        void RenderSkybox(const vk::raii::CommandBuffer& command_buffer);

        UUID GetObj2AttachmentMatID() { return m_obj2attachment_material->uuid(); }

        friend void swap(DeferredPassBase& lhs, DeferredPassBase& rhs);

    protected:
        std::shared_ptr<Material> m_obj2attachment_material = nullptr;
        std::shared_ptr<Material> m_quad_material           = nullptr;
        Model                     m_quad_model              = nullptr;
        std::shared_ptr<Material> m_skybox_material         = nullptr;
        Model                     m_skybox_model            = nullptr;

        std::shared_ptr<ImageData> m_color_attachment    = nullptr;
        std::shared_ptr<ImageData> m_normal_attachment   = nullptr;
        std::shared_ptr<ImageData> m_position_attachment = nullptr;

        LightDataBlock  m_LightDatas;
        LightSpawnBlock m_LightInfos;

        std::shared_ptr<ImageData> m_depth_attachment = nullptr;

        std::string m_pass_names[2];
        int         draw_call[2] = {0, 0};
    };
} // namespace Meow