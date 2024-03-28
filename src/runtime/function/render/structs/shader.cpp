#include "shader.h"

#include "core/log/log.h"
#include "function/global/runtime_global_context.h"
#include "function/render/structs/vertex_attribute.h"

namespace Meow
{
    Shader::Shader(vk::raii::PhysicalDevice const& gpu,
                   vk::raii::Device const&         logical_device,
                   vk::raii::DescriptorPool const& descriptor_pool,
                   vk::raii::RenderPass const&     render_pass,
                   std::string                     vert_shader_file_path,
                   std::string                     frag_shader_file_path,
                   std::string                     geom_shader_file_path,
                   std::string                     comp_shader_file_path,
                   std::string                     tesc_shader_file_path,
                   std::string                     tese_shader_file_path)
    {
        vk::raii::ShaderModule vert_shader_module = nullptr;
        vk::raii::ShaderModule frag_shader_module = nullptr;
        vk::raii::ShaderModule geom_shader_module = nullptr;
        vk::raii::ShaderModule comp_shader_module = nullptr;
        vk::raii::ShaderModule tesc_shader_module = nullptr;
        vk::raii::ShaderModule tese_shader_module = nullptr;

        std::vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_stage_create_infos;

        CreateShaderModuleAndGetMeta(logical_device,
                                     vert_shader_module,
                                     vert_shader_file_path,
                                     vk::ShaderStageFlagBits::eVertex,
                                     pipeline_shader_stage_create_infos);
        CreateShaderModuleAndGetMeta(logical_device,
                                     frag_shader_module,
                                     frag_shader_file_path,
                                     vk::ShaderStageFlagBits::eFragment,
                                     pipeline_shader_stage_create_infos);
        CreateShaderModuleAndGetMeta(logical_device,
                                     geom_shader_module,
                                     geom_shader_file_path,
                                     vk::ShaderStageFlagBits::eGeometry,
                                     pipeline_shader_stage_create_infos);
        CreateShaderModuleAndGetMeta(logical_device,
                                     comp_shader_module,
                                     comp_shader_file_path,
                                     vk::ShaderStageFlagBits::eCompute,
                                     pipeline_shader_stage_create_infos);
        CreateShaderModuleAndGetMeta(logical_device,
                                     tesc_shader_module,
                                     tesc_shader_file_path,
                                     vk::ShaderStageFlagBits::eTessellationControl,
                                     pipeline_shader_stage_create_infos);
        CreateShaderModuleAndGetMeta(logical_device,
                                     tese_shader_module,
                                     tese_shader_file_path,
                                     vk::ShaderStageFlagBits::eTessellationEvaluation,
                                     pipeline_shader_stage_create_infos);

        GenerateInputInfo();
        GenerateLayout(logical_device);

        // TODO: temp vertex layout
        // should get from analysis?
        vk::raii::PipelineCache pipeline_cache(logical_device, vk::PipelineCacheCreateInfo());

        std::vector<std::pair<vk::Format, uint32_t>> vertex_input_attribute_format_offset;
        uint32_t                                     curr_offset = 0;
        for (int32_t i = 0; i < per_vertex_attributes.size(); i++)
        {
            vertex_input_attribute_format_offset.emplace_back(VertexAttributeToVkFormat(per_vertex_attributes[i]),
                                                              curr_offset);
            curr_offset += VertexAttributeToSize(per_vertex_attributes[i]);
        }

        graphics_pipeline = MakeGraphicsPipeline(logical_device,
                                                 pipeline_cache,
                                                 vert_shader_module,
                                                 nullptr,
                                                 frag_shader_module,
                                                 nullptr,
                                                 VertexAttributesToSize(per_vertex_attributes),
                                                 vertex_input_attribute_format_offset,
                                                 vk::FrontFace::eClockwise,
                                                 true,
                                                 pipeline_layout,
                                                 render_pass);

        AllocateDescriptorSet(logical_device, descriptor_pool);
    }

    bool Shader::CreateShaderModuleAndGetMeta(
        vk::raii::Device const&                         logical_device,
        vk::raii::ShaderModule&                         shader_module,
        const std::string&                              shader_file_path,
        vk::ShaderStageFlagBits                         stage,
        std::vector<vk::PipelineShaderStageCreateInfo>& pipeline_shader_stage_create_infos)
    {
        if (shader_file_path.empty())
        {
            shader_module = nullptr;
            return false;
        }

        auto [data_ptr, data_size] = g_runtime_global_context.file_system.get()->ReadBinaryFile(shader_file_path);

        shader_module =
            vk::raii::ShaderModule(logical_device, {vk::ShaderModuleCreateFlags(), data_size, (uint32_t*)data_ptr});

        // store stage create info for creating pipeline

        pipeline_shader_stage_create_infos.emplace_back(
            vk::PipelineShaderStageCreateFlags {}, stage, *shader_module, "main", nullptr);

        // Cross compile spv to get meta information

        spirv_cross::Compiler        compiler((uint32_t*)data_ptr, data_size / sizeof(uint32_t));
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        GetAttachmentsMeta(compiler, resources, stage);
        GetUniformBuffersMeta(compiler, resources, stage);
        GetTexturesMeta(compiler, resources, stage);
        GetStorageImagesMeta(compiler, resources, stage);
        GetInputMeta(compiler, resources, stage);
        GetStorageBuffersMeta(compiler, resources, stage);

        delete[] data_ptr;
        data_ptr = nullptr;

        return true;
    }

    void Shader::GetAttachmentsMeta(spirv_cross::Compiler&        compiler,
                                    spirv_cross::ShaderResources& resources,
                                    vk::ShaderStageFlags          stageFlags)
    {
        for (int32_t i = 0; i < resources.subpass_inputs.size(); ++i)
        {
            spirv_cross::Resource& res       = resources.subpass_inputs[i];
            spirv_cross::SPIRType  type      = compiler.get_type(res.type_id);
            spirv_cross::SPIRType  base_type = compiler.get_type(res.base_type_id);
            const std::string&     var_name  = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding set_layout_binding {
                binding, vk::DescriptorType::eInputAttachment, 1, stageFlags, nullptr};

            set_layouts_meta.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            // store mapping from image name to ImageMeta
            auto it = image_meta_map.find(var_name);
            if (it == image_meta_map.end())
            {
                ImageMeta image_meta      = {};
                image_meta.set            = set;
                image_meta.binding        = binding;
                image_meta.stageFlags     = stageFlags;
                image_meta.descriptorType = set_layout_binding.descriptorType;
                image_meta_map.insert(std::make_pair(var_name, image_meta));
            }
            else
            {
                it->second.stageFlags |= stageFlags;
            }
        }
    }

    void Shader::GetUniformBuffersMeta(spirv_cross::Compiler&        compiler,
                                       spirv_cross::ShaderResources& resources,
                                       vk::ShaderStageFlags          stageFlags)
    {
        for (int32_t i = 0; i < resources.uniform_buffers.size(); ++i)
        {
            spirv_cross::Resource& res                     = resources.uniform_buffers[i];
            spirv_cross::SPIRType  type                    = compiler.get_type(res.type_id);
            spirv_cross::SPIRType  base_type               = compiler.get_type(res.base_type_id);
            const std::string&     var_name                = compiler.get_name(res.id);
            const std::string&     type_name               = compiler.get_name(res.base_type_id);
            uint32_t               uniformBufferStructSize = (uint32_t)compiler.get_declared_struct_size(type);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            // Convention: If expecting to use dynamic uniform buffer,
            // uniform name in shader should contain "Dynamic", for example:
            // [layout (binding = 0) uniform MVPDynamicBlock]
            vk::DescriptorSetLayoutBinding set_layout_binding {
                binding,
                (type_name.find("Dynamic") != std::string::npos || use_dynamic_uniform_buffer) ?
                    vk::DescriptorType::eUniformBufferDynamic :
                    vk::DescriptorType::eUniformBuffer,
                1,
                stageFlags,
                nullptr};

            set_layouts_meta.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            // store mapping from uniform buffer name to BufferMeta
            auto it = buffer_meta_map.find(var_name);
            if (it == buffer_meta_map.end())
            {
                BufferMeta buffer_meta     = {};
                buffer_meta.set            = set;
                buffer_meta.binding        = binding;
                buffer_meta.bufferSize     = uniformBufferStructSize;
                buffer_meta.stageFlags     = stageFlags;
                buffer_meta.descriptorType = set_layout_binding.descriptorType;
                buffer_meta_map.insert(std::make_pair(var_name, buffer_meta));
            }
            else
            {
                it->second.stageFlags |= set_layout_binding.stageFlags;
            }
        }
    }

    void Shader::GetTexturesMeta(spirv_cross::Compiler&        compiler,
                                 spirv_cross::ShaderResources& resources,
                                 vk::ShaderStageFlags          stageFlags)
    {
        for (int32_t i = 0; i < resources.sampled_images.size(); ++i)
        {
            spirv_cross::Resource& res       = resources.sampled_images[i];
            spirv_cross::SPIRType  type      = compiler.get_type(res.type_id);
            spirv_cross::SPIRType  base_type = compiler.get_type(res.base_type_id);
            const std::string&     var_name  = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding set_layout_binding {
                binding, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags, nullptr};

            set_layouts_meta.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            auto it = image_meta_map.find(var_name);
            if (it == image_meta_map.end())
            {
                ImageMeta image_meta      = {};
                image_meta.set            = set;
                image_meta.binding        = binding;
                image_meta.stageFlags     = stageFlags;
                image_meta.descriptorType = set_layout_binding.descriptorType;
                image_meta_map.insert(std::make_pair(var_name, image_meta));
            }
            else
            {
                it->second.stageFlags |= stageFlags;
            }
        }
    }

    void Shader::GetInputMeta(spirv_cross::Compiler&        compiler,
                              spirv_cross::ShaderResources& resources,
                              vk::ShaderStageFlags          stageFlags)
    {
        if (stageFlags != vk::ShaderStageFlagBits::eVertex)
        {
            return;
        }

        for (int32_t i = 0; i < resources.stage_inputs.size(); ++i)
        {
            spirv_cross::Resource& res                  = resources.stage_inputs[i];
            spirv_cross::SPIRType  type                 = compiler.get_type(res.type_id);
            const std::string&     var_name             = compiler.get_name(res.id);
            int32_t                input_attribute_size = type.vecsize;

            // Convection: input vertex name should be certain name, for example:
            // inPosition, inUV0, ...
            VertexAttribute attribute = StringToVertexAttribute(var_name.c_str());
            if (attribute == VertexAttribute::VA_None)
            {
                if (input_attribute_size == 1)
                {
                    attribute = VertexAttribute::VA_InstanceFloat1;
                }
                else if (input_attribute_size == 2)
                {
                    attribute = VertexAttribute::VA_InstanceFloat2;
                }
                else if (input_attribute_size == 3)
                {
                    attribute = VertexAttribute::VA_InstanceFloat3;
                }
                else if (input_attribute_size == 4)
                {
                    attribute = VertexAttribute::VA_InstanceFloat4;
                }
                // EDITOR_ERROR("Not found attribute : %s, treat as instance attribute : %d.", var_name.c_str(),
                // int32(attribute));
            }

            // store tuple of input attribute and its location
            // location must be continous
            int32_t             location              = compiler.get_decoration(res.id, spv::DecorationLocation);
            VertexAttributeMeta vertex_attribute_meta = {};
            vertex_attribute_meta.location            = location;
            vertex_attribute_meta.attribute           = attribute;
            vertex_attribute_metas.push_back(vertex_attribute_meta);
        }
    }

    void Shader::GetStorageBuffersMeta(spirv_cross::Compiler&        compiler,
                                       spirv_cross::ShaderResources& resources,
                                       vk::ShaderStageFlags          stageFlags)
    {
        for (int32_t i = 0; i < resources.storage_buffers.size(); ++i)
        {
            spirv_cross::Resource& res       = resources.storage_buffers[i];
            spirv_cross::SPIRType  type      = compiler.get_type(res.type_id);
            spirv_cross::SPIRType  base_type = compiler.get_type(res.base_type_id);
            const std::string&     var_name  = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding set_layout_binding = {
                binding, vk::DescriptorType::eStorageBuffer, 1, stageFlags, nullptr};

            set_layouts_meta.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            // store mapping from storage buffer name to BufferMeta
            auto it = buffer_meta_map.find(var_name);
            if (it == buffer_meta_map.end())
            {
                BufferMeta buffer_meta     = {};
                buffer_meta.set            = set;
                buffer_meta.binding        = binding;
                buffer_meta.bufferSize     = 0;
                buffer_meta.stageFlags     = stageFlags;
                buffer_meta.descriptorType = set_layout_binding.descriptorType;
                buffer_meta_map.insert(std::make_pair(var_name, buffer_meta));
            }
            else
            {
                it->second.stageFlags = it->second.stageFlags | set_layout_binding.stageFlags;
            }
        }
    }

    void Shader::GetStorageImagesMeta(spirv_cross::Compiler&        compiler,
                                      spirv_cross::ShaderResources& resources,
                                      vk::ShaderStageFlags          stageFlags)
    {
        for (int32_t i = 0; i < resources.storage_images.size(); ++i)
        {
            spirv_cross::Resource& res       = resources.storage_images[i];
            spirv_cross::SPIRType  type      = compiler.get_type(res.type_id);
            spirv_cross::SPIRType  base_type = compiler.get_type(res.base_type_id);
            const std::string&     var_name  = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding set_layout_binding = {
                binding, vk::DescriptorType::eStorageImage, 1, stageFlags, nullptr};

            set_layouts_meta.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            // store mapping from storage buffer name to BufferMeta
            auto it = image_meta_map.find(var_name);
            if (it == image_meta_map.end())
            {
                ImageMeta image_meta      = {};
                image_meta.set            = set;
                image_meta.binding        = binding;
                image_meta.stageFlags     = stageFlags;
                image_meta.descriptorType = set_layout_binding.descriptorType;
                image_meta_map.insert(std::make_pair(var_name, image_meta));
            }
            else
            {
                it->second.stageFlags |= stageFlags;
            }
        }
    }

    void Shader::GenerateInputInfo()
    {
        // sort input_attributes according to location
        std::sort(
            vertex_attribute_metas.begin(),
            vertex_attribute_metas.end(),
            [](const VertexAttributeMeta& a, const VertexAttributeMeta& b) -> bool { return a.location < b.location; });

        // sort input_attributes to per_vertex_attributes and instances_attributes
        for (int32_t i = 0; i < vertex_attribute_metas.size(); ++i)
        {
            VertexAttribute attribute = vertex_attribute_metas[i].attribute;
            if (attribute == VA_InstanceFloat1 || attribute == VA_InstanceFloat2 || attribute == VA_InstanceFloat3 ||
                attribute == VA_InstanceFloat4)
            {
                instances_attributes.push_back(attribute);
            }
            else
            {
                per_vertex_attributes.push_back(attribute);
            }
        }

        // generate Bindinfo
        // first is per_vertex_input_binding
        // second is instanceInputBinding
        input_bindings.resize(0);
        if (per_vertex_attributes.size() > 0)
        {
            uint32_t stride = 0;
            for (int32_t i = 0; i < per_vertex_attributes.size(); ++i)
            {
                stride += VertexAttributeToSize(per_vertex_attributes[i]);
            }
            vk::VertexInputBindingDescription per_vertex_input_binding {0, stride, vk::VertexInputRate::eVertex};
            input_bindings.push_back(per_vertex_input_binding);
        }

        if (instances_attributes.size() > 0)
        {
            uint32_t stride = 0;
            for (int32_t i = 0; i < instances_attributes.size(); ++i)
            {
                stride += VertexAttributeToSize(instances_attributes[i]);
            }
            vk::VertexInputBindingDescription instanceInputBinding {1, stride, vk::VertexInputRate::eInstance};
            input_bindings.push_back(instanceInputBinding);
        }

        // generate attributes info
        // first is per_vertex_attributes
        // second is instances_attributes
        uint32_t location = 0;
        if (per_vertex_attributes.size() > 0)
        {
            uint32_t offset = 0;
            for (int32_t i = 0; i < per_vertex_attributes.size(); ++i)
            {
                vk::VertexInputAttributeDescription input_attribute {
                    0, location, VertexAttributeToVkFormat(per_vertex_attributes[i]), offset};
                offset += VertexAttributeToSize(per_vertex_attributes[i]);
                input_attributes.push_back(input_attribute);

                location += 1;
            }
        }

        if (instances_attributes.size() > 0)
        {
            uint32_t offset = 0;
            for (int32_t i = 0; i < instances_attributes.size(); ++i)
            {
                vk::VertexInputAttributeDescription input_attribute {
                    1, location, VertexAttributeToVkFormat(instances_attributes[i]), offset};
                offset += VertexAttributeToSize(instances_attributes[i]);
                input_attributes.push_back(input_attribute);

                location += 1;
            }
        }
    }

    void Shader::GenerateLayout(vk::raii::Device const& raii_logical_device)
    {
        std::vector<DescriptorSetLayoutMeta>& set_layout_metas = set_layouts_meta.set_layout_metas;

        // first sort according to set
        std::sort(
            set_layout_metas.begin(),
            set_layout_metas.end(),
            [](const DescriptorSetLayoutMeta& a, const DescriptorSetLayoutMeta& b) -> bool { return a.set < b.set; });

        // first sort according to binding
        for (int32_t i = 0; i < set_layout_metas.size(); ++i)
        {
            std::vector<vk::DescriptorSetLayoutBinding>& bindings = set_layout_metas[i].bindings;
            std::sort(bindings.begin(),
                      bindings.end(),
                      [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) -> bool {
                          return a.binding < b.binding;
                      });
        }

        // support multiple descriptor set layout
        for (int32_t i = 0; i < set_layout_metas.size(); ++i)
        {
            DescriptorSetLayoutMeta& set_layout_meta = set_layout_metas[i];

            vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(vk::DescriptorSetLayoutCreateFlags {},
                                                                                set_layout_meta.bindings);

            vk::DescriptorSetLayout setLayout;
            raii_logical_device.getDispatcher()->vkCreateDescriptorSetLayout(
                static_cast<VkDevice>(*raii_logical_device),
                reinterpret_cast<const VkDescriptorSetLayoutCreateInfo*>(&descriptor_set_layout_create_info),
                nullptr,
                reinterpret_cast<VkDescriptorSetLayout*>(&setLayout));

            descriptor_set_layouts.push_back(setLayout);
        }

        vk::PipelineLayoutCreateInfo pipeline_layout_create_info(
            {}, (uint32_t)descriptor_set_layouts.size(), descriptor_set_layouts.data());
        pipeline_layout = vk::raii::PipelineLayout(raii_logical_device, pipeline_layout_create_info);
    }

    void Shader::AllocateDescriptorSet(vk::raii::Device const&         logical_device,
                                       vk::raii::DescriptorPool const& descriptor_pool)
    {
        // TODO: allocate descriptor set from a dynamic allocator
        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(
            *descriptor_pool, descriptor_set_layouts.size(), descriptor_set_layouts.data());
        descriptor_sets = vk::raii::DescriptorSets(logical_device, descriptor_set_allocate_info);
    }

    void Shader::PushBufferWrite(const std::string&          name,
                                 vk::raii::Buffer const&     buffer,
                                 vk::raii::BufferView const* raii_buffer_view)
    {
        auto it = set_layouts_meta.binding_meta_map.find(name);
        if (it == set_layouts_meta.binding_meta_map.end())
        {
            // TODO: support log format
            // EDITOR_ERROR("Failed write buffer, %s not found!", name.c_str());
            return;
        }

        auto bindInfo = it->second;

        // Default is offset = 0, buffer size = whole size
        // Maybe it needs to be configurable?
        descriptor_buffer_infos.emplace_back(*buffer, 0, VK_WHOLE_SIZE);

        // TODO: store buffer view in an vector
        vk::BufferView buffer_view;
        if (raii_buffer_view)
        {
            buffer_view = **raii_buffer_view;
        }

        write_descriptor_sets.emplace_back(
            *descriptor_sets[bindInfo.set],                                     // dstSet
            bindInfo.binding,                                                   // dstBinding
            0,                                                                  // dstArrayElement
            1,                                                                  // descriptorCount
            set_layouts_meta.GetDescriptorType(bindInfo.set, bindInfo.binding), // descriptorType
            nullptr,                                                            // pImageInfo
            &descriptor_buffer_infos.back(),                                    // pBufferInfo
            raii_buffer_view ? &buffer_view : nullptr                           // pTexelBufferView
        );

        int test = 1;
    }

    void Shader::PushImageWrite(const std::string& name, TextureData& texture_data)
    {
        auto it = set_layouts_meta.binding_meta_map.find(name);
        if (it == set_layouts_meta.binding_meta_map.end())
        {
            // TODO: support log format
            // EDITOR_ERROR("Failed write buffer, %s not found!", name.c_str());
            return;
        }

        auto bindInfo = it->second;

        descriptor_image_infos.emplace_back(
            *texture_data.sampler, *texture_data.image_data.image_view, vk::ImageLayout::eShaderReadOnlyOptimal);

        write_descriptor_sets.emplace_back(
            *descriptor_sets[bindInfo.set],                                     // dstSet
            bindInfo.binding,                                                   // dstBinding
            0,                                                                  // dstArrayElement
            1,                                                                  // descriptorCount
            set_layouts_meta.GetDescriptorType(bindInfo.set, bindInfo.binding), // descriptorType
            &descriptor_image_infos.back(),                                     // pImageInfo
            nullptr,                                                            // pBufferInfo
            nullptr                                                             // pTexelBufferView
        );
    }

    void Shader::UpdateDescriptorSets(vk::raii::Device const& logical_device)
    {
        logical_device.updateDescriptorSets(write_descriptor_sets, nullptr);
        descriptor_buffer_infos.clear();
        descriptor_image_infos.clear();
        write_descriptor_sets.clear();
    }

    void Shader::Bind(vk::raii::CommandBuffer const& command_buffer)
    {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);

        // convert type from vk::raii::DescriptorSet to vk::DescriptorSet

        std::vector<vk::DescriptorSet> _descriptor_sets(descriptor_sets.size());
        for (size_t i = 0; i < descriptor_sets.size(); ++i)
        {
            _descriptor_sets[i] = *descriptor_sets[i];
        }

        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0, _descriptor_sets, nullptr);
    }
} // namespace Meow