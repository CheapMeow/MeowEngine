#include "particle_system.h"

#include "gpu_particle_2d.h"

#include <random>

namespace Meow
{
    ParticleSystem::ParticleSystem() {}
    ParticleSystem::~ParticleSystem() {}

    void ParticleSystem::Start() { AddGPUParticle2D(1000); }

    void ParticleSystem::AddGPUParticle2D(uint32_t particle_count)
    {
        auto gpu_particle = std::make_shared<GPUParticle2D>(particle_count);
        m_gpu_particles.push_back(gpu_particle);
        
        std::vector<GPUParticleData2D> particle_data(particle_count);

        std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (auto& data : particle_data)
        {
            data.position = glm::vec2(0.5, 0.5);
            data.velocity = glm::vec2(dist(rng), dist(rng));
            data.color = glm::vec4(dist(rng), dist(rng), dist(rng), dist(rng));
        }

        gpu_particle->UploadParticleData(particle_data);
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
