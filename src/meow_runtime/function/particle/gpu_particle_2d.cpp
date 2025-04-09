#include "gpu_particle_2d.h"

#include "function/global/runtime_context.h"
#include "function/render/material/material_factory.h"
#include "function/render/material/shader_factory.h"

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

        m_particle_storage_buffer_per_frame.resize(k_max_frames_in_flight);
        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
        {
            m_particle_storage_buffer_per_frame[i] = StorageBuffer(physical_device,
                                                                   logical_device,
                                                                   onetime_submit_command_pool,
                                                                   graphics_queue,
                                                                   sizeof(GPUParticleData2D) * particle_count);
        }

        ShaderFactory   shader_factory;
        MaterialFactory material_factory;

        auto particle_shader = shader_factory.clear().SetComputeShader("builtin/shaders/particle.comp.spv").Create();

        m_particle_comp_material = std::make_shared<Material>(particle_shader);
        g_runtime_context.resource_system->Register(m_particle_comp_material);
        material_factory.Init(particle_shader.get());
        material_factory.CreateComputePipeline(logical_device, particle_shader.get(), m_particle_comp_material.get());

        // TODO: multiple gpu particles reuse same pipeline,
        // but different descriptor set
        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
        {
            m_particle_comp_material->BindBufferToDescriptorSet(
                "ParticleSSBOIn",
                m_particle_storage_buffer_per_frame[(i - 1) % k_max_frames_in_flight].buffer,
                VK_WHOLE_SIZE,
                nullptr,
                (i - 1) % k_max_frames_in_flight);
            m_particle_comp_material->BindBufferToDescriptorSet(
                "ParticleSSBOOut", m_particle_storage_buffer_per_frame[i].buffer, VK_WHOLE_SIZE, nullptr, i);
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
            m_particle_storage_buffer_per_frame[i].Upload(
                physical_device, logical_device, onetime_submit_command_pool, graphics_queue, m_particle_data, 0);
        }
    }

    void GPUParticle2D::UpdateUniformBuffer()
    {
        float dt = g_runtime_context.time_system->GetDeltaTime();

        // TODO: multiple gpu particles reuse same pipeline,
        // but different descriptor set
        m_particle_comp_material->PopulateUniformBuffer("ubo", &dt, sizeof(float));
    }

    void GPUParticle2D::BindPipeline(const vk::raii::CommandBuffer& command_buffer)
    {
        m_particle_comp_material->BindPipeline(command_buffer);
    }

    void GPUParticle2D::BindDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index)
    {
        m_particle_comp_material->BindDescriptorSetToPipeline(command_buffer, 0, 1, 0, false, frame_index);
    }
} // namespace Meow