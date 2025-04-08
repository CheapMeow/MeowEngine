#include "particle_system.h"

#include "gpu_particle_2d.h"

namespace Meow
{
    ParticleSystem::ParticleSystem() {}
    ParticleSystem::~ParticleSystem() {}

    void ParticleSystem::AddGPUParticle2D(uint32_t particle_count)
    {
        m_gpu_particles.push_back(std::make_shared<GPUParticle2D>(particle_count));
    }

    std::shared_ptr<GPUParticleBase> ParticleSystem::GetGPUParticle(uint32_t index)
    {
        if (index < m_gpu_particles.size())
        {
            return m_gpu_particles[index];
        }
        return nullptr;
    }
} // namespace Meow
