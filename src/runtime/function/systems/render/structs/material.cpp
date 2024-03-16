#include "material.h"

#include "function/global/runtime_global_context.h"
#include "function/systems/render/structs/vertex_attribute.h"
#include "function/systems/render/utils/vulkan_update_utils.h"

namespace Meow
{
    Material::Material(vk::raii::PhysicalDevice const& gpu,
                       vk::raii::Device const&         logical_device,
                       vk::raii::DescriptorPool const& descriptor_pool,
                       vk::raii::RenderPass const&     render_pass,
                       std::string                     vert_shader_file_path,
                       std::string                     frag_shader_file_path,
                       std::shared_ptr<TextureData>    diffuse_texture_)
        : diffuse_texture(diffuse_texture_)
    {
        CreateDescriptorSetLayout(logical_device);

        vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, *descriptor_set_layout);
        pipeline_layout = vk::raii::PipelineLayout(logical_device, pipeline_layout_create_info);

        // TODO: temp Shader
        auto [data_ptr, data_size] = g_runtime_global_context.file_system.get()->ReadBinaryFile(vert_shader_file_path);
        vk::raii::ShaderModule vertex_shader_module(logical_device,
                                                    {vk::ShaderModuleCreateFlags(), data_size, (uint32_t*)data_ptr});
        delete[] data_ptr;
        data_ptr = nullptr;

        auto [data_ptr2, data_size2] =
            g_runtime_global_context.file_system.get()->ReadBinaryFile(frag_shader_file_path);
        vk::raii::ShaderModule fragment_shader_module(
            logical_device, {vk::ShaderModuleCreateFlags(), data_size2, (uint32_t*)data_ptr2});
        delete[] data_ptr2;
        data_ptr2 = nullptr;

        // TODO: temp vertex layout
        vk::raii::PipelineCache pipeline_cache(logical_device, vk::PipelineCacheCreateInfo());
        graphics_pipeline =
            MakeGraphicsPipeline(logical_device,
                                 pipeline_cache,
                                 vertex_shader_module,
                                 nullptr,
                                 fragment_shader_module,
                                 nullptr,
                                 VertexAttributesToSize({VertexAttribute::VA_Position, VertexAttribute::VA_Normal}),
                                 {{vk::Format::eR32G32B32Sfloat, 0},
                                  {vk::Format::eR32G32B32Sfloat, 12},
                                  {vk::Format::eR32G32B32Sfloat, 24},
                                  {vk::Format::eR32G32Sfloat, 36}},
                                 vk::FrontFace::eClockwise,
                                 true,
                                 pipeline_layout,
                                 render_pass);

        CreateUniformBuffer(gpu, logical_device);
        CreateDescriptorSet(logical_device, descriptor_pool);
    }

    void Material::CreateDescriptorSetLayout(vk::raii::Device const& logical_device)
    {
        // TODO: descriptor pool size is determined by analysis of shader?
        descriptor_set_layout = MakeDescriptorSetLayout(
            logical_device,
            {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
             {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}});
    }

    void Material::CreateUniformBuffer(vk::raii::PhysicalDevice const& gpu, vk::raii::Device const& logical_device)
    {
        // TODO: UBOData is temp
        uniform_buffer_data = BufferData(gpu, logical_device, sizeof(UBOData), vk::BufferUsageFlagBits::eUniformBuffer);
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::string                     object_name = "Uniform Buffer";
        vk::DebugUtilsObjectNameInfoEXT name_info   = {
            vk::ObjectType::eBuffer, GetVulkanHandle(*uniform_buffer_data.buffer), object_name.c_str(), nullptr};
        logical_device.setDebugUtilsObjectNameEXT(name_info);
        object_name = "Uniform Buffer Device Memory";
        name_info   = {vk::ObjectType::eDeviceMemory,
                       GetVulkanHandle(*uniform_buffer_data.device_memory),
                       object_name.c_str(),
                       nullptr};
        logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void Material::CreateDescriptorSet(vk::raii::Device const&         logical_device,
                                       vk::raii::DescriptorPool const& descriptor_pool)
    {
        // allocate a descriptor set
        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(*descriptor_pool, *descriptor_set_layout);
        descriptor_set = std::move(vk::raii::DescriptorSets(logical_device, descriptor_set_allocate_info).front());

        // TODO: Uniform set is temp
        // It should be generated from analysis of shader?
        // And this should be done by material?
        UpdateDescriptorSets(
            logical_device, // device
            descriptor_set, // descriptor_set
            {
                {vk::DescriptorType::eUniformBuffer, uniform_buffer_data.buffer, VK_WHOLE_SIZE, nullptr},
            },                   // buffer_data
            {*diffuse_texture}); // texture_data
    }

    void Material::Bind(vk::raii::CommandBuffer const& command_buffer)
    {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);
        // TODO: temp {*descriptor_sets[0], *descriptor_sets[1]}
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0, {*descriptor_set}, nullptr);
    }

    void Material::UpdateUniformBuffer(UBOData ubo_data) { CopyToDevice(uniform_buffer_data.device_memory, ubo_data); }
} // namespace Meow