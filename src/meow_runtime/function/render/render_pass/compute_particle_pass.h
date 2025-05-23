#pragma once

#include "function/particle/gpu_particle_2d.h"
#include "function/render/material/material.h"
#include "function/render/material/shader.h"
#include "function/render/model/model.hpp"
#include "function/render/render_pass/render_pass_base.h"
#include "function/render/utils/vulkan_debug_utils.h"

namespace Meow
{
    class ComputeParticlePass : public RenderPassBase
    {
    public:
        ComputeParticlePass(std::nullptr_t)
            : RenderPassBase(nullptr)
        {}

        ComputeParticlePass()
            : RenderPassBase()
        {}

        ComputeParticlePass(SurfaceData& surface_data);

        ComputeParticlePass(ComputeParticlePass&& rhs) noexcept
            : RenderPassBase(nullptr)
        {
            swap(*this, rhs);
        }

        ComputeParticlePass& operator=(ComputeParticlePass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ~ComputeParticlePass() override = default;

        void CreateRenderPass();
        void CreateMaterial();

        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void RecordComputeCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) override;
        void RecordGraphicsCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) override;

        friend void swap(ComputeParticlePass& lhs, ComputeParticlePass& rhs);

    protected:
        std::shared_ptr<Material>      m_particle_render_material = nullptr;
        std::shared_ptr<GPUParticle2D> m_gpu_particle_2d          = nullptr;

        std::string m_pass_names[2];
        int         draw_call[2] = {0, 0};
    };
} // namespace Meow