#pragma once

#include "function/render/render_pass/render_pass.h"
#include "function/render/structs/material.h"

namespace Meow
{
    class ForwardLightingPass : public RenderPass
    {
    public:
        ForwardLightingPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        ForwardLightingPass(ForwardLightingPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        ForwardLightingPass& operator=(ForwardLightingPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ForwardLightingPass();

        ~ForwardLightingPass() override = default;

        void CreateMaterial();

        void UpdateUniformBuffer() override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        void AfterPresent() override;

        friend void swap(ForwardLightingPass& lhs, ForwardLightingPass& rhs);

    private:
        Material m_forward_lighting_mat = nullptr;
        Material m_skybox_mat           = nullptr;
    };
} // namespace Meow