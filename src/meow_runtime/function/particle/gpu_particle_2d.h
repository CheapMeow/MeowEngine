#pragma once

#include "gpu_particle_base.h"

#include "gpu_particle_data_2d.h"

namespace Meow
{
    class GPUParticle2D : public GPUParticleBase
    {
        GPUParticle2D() = default;
        GPUParticle2D(uint32_t particle_count);
        ~GPUParticle2D() override;

        void UploadParticleData(const std::vector<GPUParticleData2D>& in_particle_data);

    private:
        std::vector<GPUParticleData2D> m_particle_data;
    };
} // namespace Meow
