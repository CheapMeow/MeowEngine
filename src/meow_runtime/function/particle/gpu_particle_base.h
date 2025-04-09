#pragma once

#include "core/base/non_copyable.h"

#include "function/render/buffer_data/storage_buffer.h"

namespace Meow
{
    class GPUParticleBase : public NonCopyable
    {
    public:
        GPUParticleBase() {}
        GPUParticleBase(uint32_t particle_count)
            : m_particle_count(particle_count)
        {}
        virtual ~GPUParticleBase() override {}

        const uint32_t              GetParticleCount() const { return m_particle_count; }
        const uint32_t              GetMaxFramesInFlight() const { return k_max_frames_in_flight; }
        std::vector<StorageBuffer>& GetParticleStorageBuffer() { return m_particle_storage_buffer_per_frame; }

    protected:
        uint32_t                   m_particle_count       = 0;
        uint32_t                   k_max_frames_in_flight = 0;
        std::vector<StorageBuffer> m_particle_storage_buffer_per_frame;
    };
} // namespace Meow