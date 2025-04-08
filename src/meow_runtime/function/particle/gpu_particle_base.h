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

    protected:
        uint32_t                   k_max_frames_in_flight = 0;
        std::vector<StorageBuffer> m_particle_storage_buffer;

        uint32_t m_particle_count = 0;
    };
} // namespace Meow