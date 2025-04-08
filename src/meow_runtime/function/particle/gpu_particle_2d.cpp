#include "gpu_particle_2d.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    GPUParticle2D::GPUParticle2D(uint32_t particle_count)
        : GPUParticleBase(particle_count)
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        k_max_frames_in_flight =
            g_runtime_context.window_system->GetCurrentFocusGraphicsWindow()->GetMaxFramesInFlight();

        m_particle_data.resize(particle_count);

        m_particle_storage_buffer.resize(k_max_frames_in_flight);
        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
        {
            m_particle_storage_buffer[i] = StorageBuffer(physical_device,
                                                         logical_device,
                                                         onetime_submit_command_pool,
                                                         graphics_queue,
                                                         sizeof(GPUParticleData2D) * particle_count);
        }
    }

    GPUParticle2D::~GPUParticle2D() {}

    void GPUParticle2D::UploadParticleData(const std::vector<GPUParticleData2D>& in_particle_data)
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        for (uint32_t i = 0; i < m_particle_data.size() && i < in_particle_data.size(); ++i)
        {
            m_particle_data[i] = in_particle_data[i];
        }

        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
        {
            m_particle_storage_buffer[i].Upload(
                physical_device, logical_device, onetime_submit_command_pool, graphics_queue, m_particle_data, 0);
        }
    }
} // namespace Meow