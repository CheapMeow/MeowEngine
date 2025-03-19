#pragma once

#include "function/render/material/material.h"
#include "function/render/material/shader.h"
#include "function/render/model/model.hpp"
#include "function/render/render_pass/render_pass.h"

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

        void RenderOpaqueMeshes(const vk::raii::CommandBuffer& command_buffer);

        void RenderSkybox(const vk::raii::CommandBuffer& command_buffer);

        void RenderTranslucentMeshes(const vk::raii::CommandBuffer& command_buffer);

        UUID GetForwardMatID() { return m_opaque_mat.uuid; }
        UUID GetTranslucentMatID() { return m_translucent_mat.uuid; }

        friend void swap(ForwardPass& lhs, ForwardPass& rhs);

    protected:
        Material m_opaque_mat      = nullptr;
        Material m_skybox_mat      = nullptr;
        Model    m_skybox_model    = nullptr;
        Material m_translucent_mat = nullptr;

        std::string m_pass_names[2];
        int         draw_call[3] = {0, 0, 0};
    };
} // namespace Meow