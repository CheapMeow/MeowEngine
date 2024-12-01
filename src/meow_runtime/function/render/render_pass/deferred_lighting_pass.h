#pragma once

#include "function/render/render_pass/render_pass.h"
#include "function/render/render_resources/model.hpp"
#include "function/render/structs/material.h"

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

    class DeferredLightingPass : public RenderPass
    {
    public:
        DeferredLightingPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        DeferredLightingPass(DeferredLightingPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        DeferredLightingPass& operator=(DeferredLightingPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        DeferredLightingPass();

        ~DeferredLightingPass() override = default;

        void CreateMaterial();

        void RefreshAttachments(ImageData& color_attachment,
                                ImageData& normal_attachment,
                                ImageData& position_attachment,
                                ImageData& depth_attachment);

        void UpdateUniformBuffer() override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        void AfterPresent() override;

        friend void swap(DeferredLightingPass& lhs, DeferredLightingPass& rhs);

    private:
        Material m_deferred_lighting_mat = nullptr;
        Model    m_quad_model            = nullptr;

        LightDataBlock  m_LightDatas;
        LightSpawnBlock m_LightInfos;
    };
} // namespace Meow