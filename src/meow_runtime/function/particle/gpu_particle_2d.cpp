#include "gpu_particle_2d.h"

#include "function/global/runtime_context.h"
#include "function/render/material/material_factory.h"

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

        MaterialFactory material_factory;

        // auto particle_shader = std::make_shared<Shader>(
        //     physical_device, logical_device, "builtin/shaders/obj.vert.spv", "builtin/shaders/obj.frag.spv");

        // m_particle_material = std::make_shared<Material>(particle_shader);
        // g_runtime_context.resource_system->Register(m_particle_material);
        // material_factory.Init(particle_shader.get(), vk::FrontFace::eClockwise);
        // material_factory.SetOpaque(true, 3);
        // material_factory.CreatePipeline(
        //     logical_device, render_pass, particle_shader.get(), m_particle_material.get(), 0);
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