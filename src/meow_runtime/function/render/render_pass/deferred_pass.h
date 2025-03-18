#pragma once

#include "function/render/render_pass/render_pass.h"
#include "function/render/render_resources/model.hpp"
#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"

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

    class DeferredPass : public RenderPass
    {
    public:
        DeferredPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        DeferredPass()
            : RenderPass()
        {}

        DeferredPass(DeferredPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        DeferredPass& operator=(DeferredPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~DeferredPass() override = default;

        void CreateMaterial();

        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void RenderGBuffer(const vk::raii::CommandBuffer& command_buffer);

        void MeshLighting(const vk::raii::CommandBuffer& command_buffer);

        void RenderSkybox(const vk::raii::CommandBuffer& command_buffer);

        friend void swap(DeferredPass& lhs, DeferredPass& rhs);

    protected:
        vk::Format m_color_format;

        Material m_obj2attachment_mat = nullptr;
        Material m_quad_mat           = nullptr;
        Model    m_quad_model         = nullptr;
        Material m_skybox_mat         = nullptr;
        Model    m_skybox_model       = nullptr;

        std::shared_ptr<ImageData> m_color_attachment    = nullptr;
        std::shared_ptr<ImageData> m_normal_attachment   = nullptr;
        std::shared_ptr<ImageData> m_position_attachment = nullptr;

        LightDataBlock  m_LightDatas;
        LightSpawnBlock m_LightInfos;

        std::string m_pass_names[2];
        int         draw_call[2] = {0, 0};
    };
} // namespace Meow