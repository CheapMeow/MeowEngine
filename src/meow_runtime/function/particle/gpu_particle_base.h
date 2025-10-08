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
        ~GPUParticleBase() override {}

        virtual void UpdateUniformBuffer(uint32_t frame_index) {}
        virtual void BindPipeline(const vk::raii::CommandBuffer& command_buffer) {}
        virtual void BindDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) {}
        virtual void Dispatch(const vk::raii::CommandBuffer& command_buffer) {}

        const uint32_t              GetParticleCount() const { return m_particle_count; }
        std::vector<StorageBuffer>& GetParticleStorageBuffer() { return m_particle_storage_buffer_per_frame; }

    protected:
        uint32_t                   m_particle_count = 0;
        std::vector<StorageBuffer> m_particle_storage_buffer_per_frame;
    };
} // namespace Meow