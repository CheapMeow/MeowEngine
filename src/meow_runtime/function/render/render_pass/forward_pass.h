#pragma once

#include "function/render/render_pass/render_pass.h"
#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"

namespace Meow
{
    struct LightData
    {
        glm::vec4 pos[4] = {
            glm::vec4(-10.0f, 10.0f, 10.0f, 0.0f),
            glm::vec4(10.0f, 10.0f, 10.0f, 0.0f),
            glm::vec4(-10.0f, -10.0f, 10.0f, 0.0f),
            glm::vec4(10.0f, -10.0f, 10.0f, 0.0f),
        };
        glm::vec4 color[4] = {
            glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
            glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
            glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
            glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
        };
        glm::vec3 camPos;
    };

    struct PBRParam
    {
        glm::vec3 albedo;
        float     metallic;
        float     roughness;
        float     ao;
    };

    struct MVPBlock
    {
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
    };

    class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        ForwardPass()
            : RenderPass()
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

        void CreateMaterial();

        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void DrawOnly(const vk::raii::CommandBuffer& command_buffer);

        friend void swap(ForwardPass& lhs, ForwardPass& rhs);

    protected:
        Material                       m_forward_mat = nullptr;
        std::shared_ptr<UniformBuffer> m_per_scene_uniform_buffer;
        std::shared_ptr<UniformBuffer> m_light_uniform_buffer;
        std::shared_ptr<UniformBuffer> m_dynamic_uniform_buffer;

        Material                       m_skybox_mat = nullptr;
        std::shared_ptr<UniformBuffer> m_skybox_uniform_buffer;

        int draw_call = 0;
    };
} // namespace Meow