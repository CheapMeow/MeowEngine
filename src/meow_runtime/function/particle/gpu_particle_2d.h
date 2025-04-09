#pragma once

#include "gpu_particle_base.h"

#include "function/render/material/material.h"
#include "gpu_particle_data_2d.h"

namespace Meow
{
    class GPUParticle2D : public GPUParticleBase
    {
    public:
        GPUParticle2D() = default;
        GPUParticle2D(uint32_t particle_count);
        ~GPUParticle2D() override;

        void UploadParticleData(const std::vector<GPUParticleData2D>& in_particle_data);
        void UpdateUniformBuffer() override;
        void BindPipeline(const vk::raii::CommandBuffer& command_buffer) override;
        void BindDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) override;
        void Dispatch(const vk::raii::CommandBuffer& command_buffer) override;

    private:
        std::vector<GPUParticleData2D> m_particle_data;
        std::shared_ptr<Material>      m_particle_comp_material;
    };
} // namespace Meow
