#pragma once

#include "function/system.h"

#include "gpu_particle_base.h"

namespace Meow
{
    class ParticleSystem final : public System
    {
    public:
        ParticleSystem();
        ~ParticleSystem();

        void Start() override;

        void Tick(float dt) override {}

        void AddGPUParticle2D(uint32_t particle_count);

        std::shared_ptr<GPUParticleBase>                     GetGPUParticle(uint32_t index);
        const std::vector<std::shared_ptr<GPUParticleBase>>& GetGPUParticles() const { return m_gpu_particles; }

    private:
        std::vector<std::shared_ptr<GPUParticleBase>> m_gpu_particles;
    };
} // namespace Meow
