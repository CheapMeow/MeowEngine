#pragma once

#include "function/render/render_pass/render_pass.h"
#include "function/render/structs/material.h"

namespace Meow
{
    class GbufferPass : public RenderPass
    {
    public:
        GbufferPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        GbufferPass(GbufferPass&& rhs) noexcept
            : RenderPass(nullptr)
        {
            swap(*this, rhs);
        }

        GbufferPass& operator=(GbufferPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        GbufferPass();

        ~GbufferPass() override = default;

        void CreateMaterial();

        void UpdateUniformBuffer() override;

        void BeforeRender(const vk::raii::CommandBuffer& command_buffer) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        void End(const vk::raii::CommandBuffer& command_buffer, ImageData& color_attachment);

        void AfterPresent() override;

        friend void swap(GbufferPass& lhs, GbufferPass& rhs);

    private:
        Material m_gbuffer_mat = nullptr;
    };
} // namespace Meow