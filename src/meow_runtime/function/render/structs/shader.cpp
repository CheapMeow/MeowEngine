#include "shader.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    Shader::Shader(const vk::raii::PhysicalDevice& physical_device,
                   const vk::raii::Device&         logical_device,
                   DescriptorAllocatorGrowable&    descriptor_allocator,
                   std::string                     vert_shader_file_path,
                   std::string                     frag_shader_file_path,
                   std::string                     geom_shader_file_path,
                   std::string                     comp_shader_file_path,
                   std::string                     tesc_shader_file_path,
                   std::string                     tese_shader_file_path)
        : m_device(*logical_device)
        , m_dispatcher(logical_device.getDispatcher())
    {
        std::vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_stage_create_infos;

        if (CreateShaderModuleAndGetMeta(logical_device,
                                         vert_shader_module,
                                         vert_shader_file_path,
                                         vk::ShaderStageFlagBits::eVertex,
                                         pipeline_shader_stage_create_infos))
            is_vert_shader_valid = true;
        else
            is_vert_shader_valid = false;

        if (CreateShaderModuleAndGetMeta(logical_device,
                                         frag_shader_module,
                                         frag_shader_file_path,
                                         vk::ShaderStageFlagBits::eFragment,
                                         pipeline_shader_stage_create_infos))
            is_frag_shader_valid = true;
        else
            is_frag_shader_valid = false;

        if (CreateShaderModuleAndGetMeta(logical_device,
                                         geom_shader_module,
                                         geom_shader_file_path,
                                         vk::ShaderStageFlagBits::eGeometry,
                                         pipeline_shader_stage_create_infos))
            is_geom_shader_valid = true;
        else
            is_geom_shader_valid = false;

        if (CreateShaderModuleAndGetMeta(logical_device,
                                         comp_shader_module,
                                         comp_shader_file_path,
                                         vk::ShaderStageFlagBits::eCompute,
                                         pipeline_shader_stage_create_infos))
            is_comp_shader_valid = true;
        else
            is_comp_shader_valid = false;

        if (CreateShaderModuleAndGetMeta(logical_device,
                                         tesc_shader_module,
                                         tesc_shader_file_path,
                                         vk::ShaderStageFlagBits::eTessellationControl,
                                         pipeline_shader_stage_create_infos))
            is_tesc_shader_valid = true;
        else
            is_tesc_shader_valid = false;

        if (CreateShaderModuleAndGetMeta(logical_device,
                                         tese_shader_module,
                                         tese_shader_file_path,
                                         vk::ShaderStageFlagBits::eTessellationEvaluation,
                                         pipeline_shader_stage_create_infos))
            is_tese_shader_valid = true;
        else
            is_tese_shader_valid = false;

        GenerateInputInfo();
        GenerateLayout(logical_device);
        GenerateDynamicUniformBufferOffset();
        AllocateDescriptorSet(logical_device, descriptor_allocator);
    }

    bool Shader::CreateShaderModuleAndGetMeta(
        const vk::raii::Device&                         logical_device,
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

        auto [data_ptr, data_size] = g_runtime_context.file_system.get()->ReadBinaryFile(shader_file_path);

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

            set_layout_metas.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

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
            spirv_cross::Resource& res          = resources.uniform_buffers[i];
            spirv_cross::SPIRType  type         = compiler.get_type(res.type_id);
            spirv_cross::SPIRType  base_type    = compiler.get_type(res.base_type_id);
            const std::string&     var_name     = compiler.get_name(res.id);
            const std::string&     type_name    = compiler.get_name(res.base_type_id);
            uint32_t uniform_buffer_struct_size = static_cast<uint32_t>(compiler.get_declared_struct_size(type));

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding set_layout_binding {binding,
                                                               (type_name.find("Dynamic") != std::string::npos) ?
                                                                   vk::DescriptorType::eUniformBufferDynamic :
                                                                   vk::DescriptorType::eUniformBuffer,
                                                               1,
                                                               stageFlags,
                                                               nullptr};

            set_layout_metas.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            // store mapping from uniform buffer name to BufferMeta
            auto it = buffer_meta_map.find(var_name);
            if (it == buffer_meta_map.end())
            {
                BufferMeta buffer_meta     = {};
                buffer_meta.set            = set;
                buffer_meta.binding        = binding;
                buffer_meta.size           = uniform_buffer_struct_size;
                buffer_meta.stageFlags     = stageFlags;
                buffer_meta.descriptorType = set_layout_binding.descriptorType;

#ifdef MEOW_DEBUG
                buffer_meta.var_name  = var_name;
                buffer_meta.type_name = type_name;
#endif

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

            set_layout_metas.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

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
            size_t pos = var_name.find("in");
            if (pos == std::string::npos)
                continue;

            std::string        vat_name_substr = var_name.substr(pos + 2);
            VertexAttributeBit attribute       = to_enum(vat_name_substr);
            if (attribute == VertexAttributeBit::None)
            {
                if (input_attribute_size == 1)
                {
                    attribute = VertexAttributeBit::InstanceFloat1;
                }
                else if (input_attribute_size == 2)
                {
                    attribute = VertexAttributeBit::InstanceFloat2;
                }
                else if (input_attribute_size == 3)
                {
                    attribute = VertexAttributeBit::InstanceFloat3;
                }
                else if (input_attribute_size == 4)
                {
                    attribute = VertexAttributeBit::InstanceFloat4;
                }
                // EDITOR_MEOW_ERROR("Not found attribute : %s, treat as instance attribute : %d.", var_name.c_str(),
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

            set_layout_metas.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            // store mapping from storage buffer name to BufferMeta
            auto it = buffer_meta_map.find(var_name);
            if (it == buffer_meta_map.end())
            {
                BufferMeta buffer_meta     = {};
                buffer_meta.set            = set;
                buffer_meta.binding        = binding;
                buffer_meta.size           = 0;
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

            set_layout_metas.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

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

        // sort input_attributes to per_vertex_attributes and instance_attributes
        for (int32_t i = 0; i < vertex_attribute_metas.size(); ++i)
        {
            VertexAttributeBit attribute = vertex_attribute_metas[i].attribute;
            if (attribute == VertexAttributeBit::InstanceFloat1 || attribute == VertexAttributeBit::InstanceFloat2 ||
                attribute == VertexAttributeBit::InstanceFloat3 || attribute == VertexAttributeBit::InstanceFloat4)
            {
                instance_attributes |= attribute;
            }
            else
            {
                per_vertex_attributes |= attribute;
            }
        }

        // generate Bindinfo
        // first is per_vertex_input_binding
        // second is instanceInputBinding
        input_bindings.resize(0);
        if (per_vertex_attributes.count() > 0)
        {
            uint32_t                          stride = VertexAttributesToSize(per_vertex_attributes);
            vk::VertexInputBindingDescription per_vertex_input_binding {0, stride, vk::VertexInputRate::eVertex};
            input_bindings.push_back(per_vertex_input_binding);
        }

        if (instance_attributes.count() > 0)
        {
            uint32_t                          stride = VertexAttributesToSize(instance_attributes);
            vk::VertexInputBindingDescription instanceInputBinding {1, stride, vk::VertexInputRate::eInstance};
            input_bindings.push_back(instanceInputBinding);
        }

        // generate attributes info
        // first is per_vertex_attributes
        // second is instance_attributes
        uint32_t location = 0;
        if (per_vertex_attributes.count() > 0)
        {
            uint32_t offset     = 0;
            auto     attributes = per_vertex_attributes.split();
            for (int32_t i = 0; i < attributes.size(); ++i)
            {
                vk::VertexInputAttributeDescription input_attribute {
                    0, location, VertexAttributeToVkFormat(attributes[i]), offset};
                offset += VertexAttributeToSize(attributes[i]);
                input_attributes.push_back(input_attribute);

                location += 1;
            }
        }

        if (instance_attributes.count() > 0)
        {
            uint32_t offset     = 0;
            auto     attributes = instance_attributes.split();
            for (int32_t i = 0; i < attributes.size(); ++i)
            {
                vk::VertexInputAttributeDescription input_attribute {
                    1, location, VertexAttributeToVkFormat(attributes[i]), offset};
                offset += VertexAttributeToSize(attributes[i]);
                input_attributes.push_back(input_attribute);

                location += 1;
            }
        }
    }

    void Shader::GenerateLayout(const vk::raii::Device& raii_logical_device)
    {
        std::vector<DescriptorSetLayoutMeta>& metas = set_layout_metas.metas;

        // first sort according to set
        std::sort(
            metas.begin(), metas.end(), [](const DescriptorSetLayoutMeta& a, const DescriptorSetLayoutMeta& b) -> bool {
                return a.set < b.set;
            });

        // first sort according to binding
        for (int32_t i = 0; i < metas.size(); ++i)
        {
            std::vector<vk::DescriptorSetLayoutBinding>& bindings = metas[i].bindings;
            std::sort(bindings.begin(),
                      bindings.end(),
                      [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) -> bool {
                          return a.binding < b.binding;
                      });
        }

        // support multiple descriptor set layout
        for (int32_t i = 0; i < metas.size(); ++i)
        {
            DescriptorSetLayoutMeta& set_layout_meta = metas[i];

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
            {}, static_cast<uint32_t>(descriptor_set_layouts.size()), descriptor_set_layouts.data());
        pipeline_layout = vk::raii::PipelineLayout(raii_logical_device, pipeline_layout_create_info);
    }

    void Shader::GenerateDynamicUniformBufferOffset()
    {
        // metas has been sort acrroding to
        std::vector<DescriptorSetLayoutMeta>& metas = set_layout_metas.metas;

        // set uniform buffer offset index
        // for the use of dynamic uniform buffer
        // the offset index is related about set and binding
        // so it is wrong to iterate like:
        // for (auto& buffer_meta : buffer_meta_map)
        // instead, use double layer looping

        dynamic_uniform_buffer_count = 0;
        for (auto& meta : metas)
        {
            for (auto& descriptor_layout_binding : meta.bindings)
            {
                if (descriptor_layout_binding.descriptorType == vk::DescriptorType::eUniformBufferDynamic)
                {
                    for (auto& buffer_meta : buffer_meta_map)
                    {
                        if (buffer_meta.second.set == meta.set &&
                            buffer_meta.second.binding == descriptor_layout_binding.binding &&
                            buffer_meta.second.descriptorType == descriptor_layout_binding.descriptorType &&
                            buffer_meta.second.stageFlags == descriptor_layout_binding.stageFlags)
                        {
                            buffer_meta.second.dynamic_seq = dynamic_uniform_buffer_count;
                            dynamic_uniform_buffer_count++;
                            break;
                        }
                    }
                }
            }
        }
    }

    void Shader::AllocateDescriptorSet(const vk::raii::Device&      logical_device,
                                       DescriptorAllocatorGrowable& descriptor_allocator)
    {
        descriptor_sets = descriptor_allocator.Allocate(logical_device, descriptor_set_layouts);
    }

    void Shader::BindBufferToDescriptor(const vk::raii::Device&     logical_device,
                                        const std::string&          name,
                                        const vk::raii::Buffer&     buffer,
                                        vk::DeviceSize              range,
                                        const vk::raii::BufferView* raii_buffer_view)
    {
        BufferMeta* meta = nullptr;
        // If it is dynamic uniform buffer, then the buffer passed into can not use whole size
        for (auto it = buffer_meta_map.begin(); it != buffer_meta_map.end(); ++it)
        {
            if (it->first == name)
            {
                if (it->second.descriptorType == vk::DescriptorType::eUniformBuffer)
                {
                    meta = &it->second;
                    break;
                }
                else if (it->second.descriptorType == vk::DescriptorType::eUniformBufferDynamic)
                {
                    meta  = &it->second;
                    range = meta->size;
                    break;
                }
            }
        }

        if (meta == nullptr)
        {
            MEOW_ERROR("Binding buffer failed, {} not found!", name);
            return;
        }

        vk::DescriptorBufferInfo descriptor_buffer_info(*buffer, 0, range);

        // TODO: store buffer view in an vector
        vk::BufferView buffer_view;
        if (raii_buffer_view)
        {
            buffer_view = **raii_buffer_view;
        }

        vk::WriteDescriptorSet write_descriptor_set(
            *descriptor_sets[meta->set],                                  // dstSet
            meta->binding,                                                // dstBinding
            0,                                                            // dstArrayElement
            1,                                                            // descriptorCount
            set_layout_metas.GetDescriptorType(meta->set, meta->binding), // descriptorType
            nullptr,                                                      // pImageInfo
            &descriptor_buffer_info,                                      // pBufferInfo
            raii_buffer_view ? &buffer_view : nullptr                     // pTexelBufferView
        );

        logical_device.updateDescriptorSets(write_descriptor_set, nullptr);
    }

    void Shader::BindImageToDescriptor(const vk::raii::Device& logical_device,
                                       const std::string&      name,
                                       ImageData&              image_data)
    {
        auto it = set_layout_metas.binding_meta_map.find(name);
        if (it == set_layout_metas.binding_meta_map.end())
        {
            MEOW_ERROR("Writing buffer failed, {} not found!", name);
            return;
        }

        auto bindInfo = it->second;

        vk::DescriptorImageInfo descriptor_image_info(
            *image_data.sampler, *image_data.image_view, vk::ImageLayout::eShaderReadOnlyOptimal);

        vk::WriteDescriptorSet write_descriptor_set(
            *descriptor_sets[bindInfo.set],                                     // dstSet
            bindInfo.binding,                                                   // dstBinding
            0,                                                                  // dstArrayElement
            1,                                                                  // descriptorCount
            set_layout_metas.GetDescriptorType(bindInfo.set, bindInfo.binding), // descriptorType
            &descriptor_image_info,                                             // pImageInfo
            nullptr,                                                            // pBufferInfo
            nullptr                                                             // pTexelBufferView
        );

        logical_device.updateDescriptorSets(write_descriptor_set, nullptr);
    }

    void Shader::BindPerSceneDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer)
    {
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0, *descriptor_sets[0], {});
    }

    void Shader::BindPerShaderDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer)
    {
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline_layout, 1, *descriptor_sets[1], {});
    }

    void Shader::BindPerMaterialDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer)
    {
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline_layout, 2, *descriptor_sets[2], {});
    }

    void Shader::BindPerObjectDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer,
                                                      const std::vector<uint32_t>&   dynamic_offsets)
    {
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline_layout, 3, *descriptor_sets[3], dynamic_offsets);
    }
} // namespace Meow