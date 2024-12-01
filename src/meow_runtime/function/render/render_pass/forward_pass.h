#pragma once

#include "function/render/render_pass/render_pass.h"
#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"

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

        void MeshLighting(const vk::raii::CommandBuffer& command_buffer);

        friend void swap(ForwardPass& lhs, ForwardPass& rhs);

    protected:
        Material m_forward_mat = nullptr;
        Material m_skybox_mat  = nullptr;

        int draw_call = 0;
    };
} // namespace Meow