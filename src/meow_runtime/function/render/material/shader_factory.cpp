#include "shader_factory.h"
#include "function/global/runtime_context.h"

namespace Meow
{
    ShaderFactory& ShaderFactory::clear()
    {
        m_vert_shader_file_path.clear();
        m_frag_shader_file_path.clear();
        m_geom_shader_file_path.clear();
        m_comp_shader_file_path.clear();
        m_tesc_shader_file_path.clear();
        m_tese_shader_file_path.clear();
        return *this;
    }

    ShaderFactory& ShaderFactory::SetVertexShader(const std::string& vert_shader_file_path)
    {
        m_vert_shader_file_path = vert_shader_file_path;
        return *this;
    }

    ShaderFactory& ShaderFactory::SetFragmentShader(const std::string& frag_shader_file_path)
    {
        m_frag_shader_file_path = frag_shader_file_path;
        return *this;
    }

    ShaderFactory& ShaderFactory::SetGeometryShader(const std::string& geom_shader_file_path)
    {
        m_geom_shader_file_path = geom_shader_file_path;
        return *this;
    }

    ShaderFactory& ShaderFactory::SetComputeShader(const std::string& comp_shader_file_path)
    {
        m_comp_shader_file_path = comp_shader_file_path;
        return *this;
    }

    ShaderFactory& ShaderFactory::SetTessellationControlShader(const std::string& tesc_shader_file_path)
    {
        m_tesc_shader_file_path = tesc_shader_file_path;
        return *this;
    }

    ShaderFactory& ShaderFactory::SetTessellationEvaluationShader(const std::string& tese_shader_file_path)
    {
        m_tese_shader_file_path = tese_shader_file_path;
        return *this;
    }

    std::shared_ptr<Shader> ShaderFactory::Create()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        auto shader          = std::make_shared<Shader>();
        shader->m_device     = *logical_device;
        shader->m_dispatcher = logical_device.getDispatcher();

        std::vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_stage_create_infos;

        if (!m_vert_shader_file_path.empty())
        {
            shader->is_vert_shader_valid = CreateShaderModuleAndGetMeta(*shader,
                                                                        shader->vert_shader_module,
                                                                        m_vert_shader_file_path,
                                                                        vk::ShaderStageFlagBits::eVertex,
                                                                        pipeline_shader_stage_create_infos);
        }

        if (!m_frag_shader_file_path.empty())
        {
            shader->is_frag_shader_valid = CreateShaderModuleAndGetMeta(*shader,
                                                                        shader->frag_shader_module,
                                                                        m_frag_shader_file_path,
                                                                        vk::ShaderStageFlagBits::eFragment,
                                                                        pipeline_shader_stage_create_infos);
        }

        if (!m_geom_shader_file_path.empty())
        {
            shader->is_geom_shader_valid = CreateShaderModuleAndGetMeta(*shader,
                                                                        shader->geom_shader_module,
                                                                        m_geom_shader_file_path,
                                                                        vk::ShaderStageFlagBits::eGeometry,
                                                                        pipeline_shader_stage_create_infos);
        }

        if (!m_comp_shader_file_path.empty())
        {
            shader->is_comp_shader_valid = CreateShaderModuleAndGetMeta(*shader,
                                                                        shader->comp_shader_module,
                                                                        m_comp_shader_file_path,
                                                                        vk::ShaderStageFlagBits::eCompute,
                                                                        pipeline_shader_stage_create_infos);
        }

        if (!m_tesc_shader_file_path.empty())
        {
            shader->is_tesc_shader_valid = CreateShaderModuleAndGetMeta(*shader,
                                                                        shader->tesc_shader_module,
                                                                        m_tesc_shader_file_path,
                                                                        vk::ShaderStageFlagBits::eTessellationControl,
                                                                        pipeline_shader_stage_create_infos);
        }

        if (!m_tese_shader_file_path.empty())
        {
            shader->is_tese_shader_valid =
                CreateShaderModuleAndGetMeta(*shader,
                                             shader->tese_shader_module,
                                             m_tese_shader_file_path,
                                             vk::ShaderStageFlagBits::eTessellationEvaluation,
                                             pipeline_shader_stage_create_infos);
        }

        GenerateInputInfo(*shader);
        GeneratePipelineLayout(*shader);
        GenerateDynamicUniformBufferOffset(*shader);

        return shader;
    }

    bool ShaderFactory::CreateShaderModuleAndGetMeta(
        Shader&                                         shader,
        vk::raii::ShaderModule&                         shader_module,
        const std::string&                              shader_file_path,
        vk::ShaderStageFlagBits                         stage,
        std::vector<vk::PipelineShaderStageCreateInfo>& pipeline_shader_stage_create_infos)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        auto [data_ptr, data_size] = g_runtime_context.file_system.get()->ReadBinaryFile(shader_file_path);

        shader_module = vk::raii::ShaderModule(
            logical_device, vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), data_size, (uint32_t*)data_ptr));

        pipeline_shader_stage_create_infos.emplace_back(
            vk::PipelineShaderStageCreateFlags(), stage, *shader_module, "main", nullptr);

        spirv_cross::Compiler        compiler((uint32_t*)data_ptr, data_size / sizeof(uint32_t));
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        GetAttachmentsMeta(shader, compiler, resources, stage);
        GetUniformBuffersMeta(shader, compiler, resources, stage);
        GetTexturesMeta(shader, compiler, resources, stage);
        GetStorageImagesMeta(shader, compiler, resources, stage);
        GetInputMeta(shader, compiler, resources, stage);
        GetStorageBuffersMeta(shader, compiler, resources, stage);

        delete[] data_ptr;
        return true;
    }

    void ShaderFactory::GetAttachmentsMeta(Shader&                       shader,
                                           spirv_cross::Compiler&        compiler,
                                           spirv_cross::ShaderResources& resources,
                                           vk::ShaderStageFlags          stageFlags)
    {
        for (size_t i = 0; i < resources.subpass_inputs.size(); ++i)
        {
            spirv_cross::Resource& res      = resources.subpass_inputs[i];
            const std::string&     var_name = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding set_layout_binding(
                binding, vk::DescriptorType::eInputAttachment, 1, stageFlags, nullptr);

            shader.set_layout_metas.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            auto it = shader.image_meta_map.find(var_name);
            if (it == shader.image_meta_map.end())
            {
                ImageMeta image_meta;
                image_meta.set            = set;
                image_meta.binding        = binding;
                image_meta.stageFlags     = stageFlags;
                image_meta.descriptorType = set_layout_binding.descriptorType;
                shader.image_meta_map.emplace(var_name, image_meta);
            }
            else
            {
                it->second.stageFlags |= stageFlags;
            }
        }
    }

    void ShaderFactory::GetUniformBuffersMeta(Shader&                       shader,
                                              spirv_cross::Compiler&        compiler,
                                              spirv_cross::ShaderResources& resources,
                                              vk::ShaderStageFlags          stageFlags)
    {
        for (size_t i = 0; i < resources.uniform_buffers.size(); ++i)
        {
            spirv_cross::Resource& res          = resources.uniform_buffers[i];
            spirv_cross::SPIRType  type         = compiler.get_type(res.type_id);
            const std::string&     var_name     = compiler.get_name(res.id);
            const std::string&     type_name    = compiler.get_name(res.base_type_id);
            uint32_t uniform_buffer_struct_size = static_cast<uint32_t>(compiler.get_declared_struct_size(type));

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding set_layout_binding(binding,
                                                              (type_name.find("Dynamic") != std::string::npos) ?
                                                                  vk::DescriptorType::eUniformBufferDynamic :
                                                                  vk::DescriptorType::eUniformBuffer,
                                                              1,
                                                              stageFlags,
                                                              nullptr);

            shader.set_layout_metas.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            auto it = shader.buffer_meta_map.find(var_name);
            if (it == shader.buffer_meta_map.end())
            {
                BufferMeta buffer_meta;
                buffer_meta.set            = set;
                buffer_meta.binding        = binding;
                buffer_meta.size           = uniform_buffer_struct_size;
                buffer_meta.stageFlags     = stageFlags;
                buffer_meta.descriptorType = set_layout_binding.descriptorType;

#ifdef MEOW_DEBUG
                buffer_meta.var_name  = var_name;
                buffer_meta.type_name = type_name;
#endif

                shader.buffer_meta_map.emplace(var_name, buffer_meta);
            }
            else
            {
                it->second.stageFlags |= stageFlags;
            }
        }
    }

    void ShaderFactory::GetTexturesMeta(Shader&                       shader,
                                        spirv_cross::Compiler&        compiler,
                                        spirv_cross::ShaderResources& resources,
                                        vk::ShaderStageFlags          stageFlags)
    {
        for (size_t i = 0; i < resources.sampled_images.size(); ++i)
        {
            spirv_cross::Resource& res      = resources.sampled_images[i];
            const std::string&     var_name = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding set_layout_binding(
                binding, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags, nullptr);

            shader.set_layout_metas.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            auto it = shader.image_meta_map.find(var_name);
            if (it == shader.image_meta_map.end())
            {
                ImageMeta image_meta;
                image_meta.set            = set;
                image_meta.binding        = binding;
                image_meta.stageFlags     = stageFlags;
                image_meta.descriptorType = set_layout_binding.descriptorType;
                shader.image_meta_map.emplace(var_name, image_meta);
            }
            else
            {
                it->second.stageFlags |= stageFlags;
            }
        }
    }

    void ShaderFactory::GetInputMeta(Shader&                       shader,
                                     spirv_cross::Compiler&        compiler,
                                     spirv_cross::ShaderResources& resources,
                                     vk::ShaderStageFlags          stageFlags)
    {
        if (stageFlags != vk::ShaderStageFlagBits::eVertex)
            return;

        for (size_t i = 0; i < resources.stage_inputs.size(); ++i)
        {
            spirv_cross::Resource& res                  = resources.stage_inputs[i];
            spirv_cross::SPIRType  type                 = compiler.get_type(res.type_id);
            const std::string&     var_name             = compiler.get_name(res.id);
            int32_t                input_attribute_size = type.vecsize;

            size_t pos = var_name.find("in");
            if (pos == std::string::npos)
                continue;

            std::string        vat_name_substr = var_name.substr(pos + 2);
            VertexAttributeBit attribute       = to_enum(vat_name_substr);
            if (attribute == VertexAttributeBit::None)
            {
                if (input_attribute_size == 1)
                    attribute = VertexAttributeBit::InstanceFloat1;
                else if (input_attribute_size == 2)
                    attribute = VertexAttributeBit::InstanceFloat2;
                else if (input_attribute_size == 3)
                    attribute = VertexAttributeBit::InstanceFloat3;
                else if (input_attribute_size == 4)
                    attribute = VertexAttributeBit::InstanceFloat4;
            }

            int32_t             location = compiler.get_decoration(res.id, spv::DecorationLocation);
            VertexAttributeMeta vertex_attribute_meta;
            vertex_attribute_meta.location  = location;
            vertex_attribute_meta.attribute = attribute;
            shader.vertex_attribute_metas.push_back(vertex_attribute_meta);
        }
    }

    void ShaderFactory::GetStorageBuffersMeta(Shader&                       shader,
                                              spirv_cross::Compiler&        compiler,
                                              spirv_cross::ShaderResources& resources,
                                              vk::ShaderStageFlags          stageFlags)
    {
        for (size_t i = 0; i < resources.storage_buffers.size(); ++i)
        {
            spirv_cross::Resource& res      = resources.storage_buffers[i];
            const std::string&     var_name = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding set_layout_binding(
                binding, vk::DescriptorType::eStorageBuffer, 1, stageFlags, nullptr);

            shader.set_layout_metas.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            auto it = shader.buffer_meta_map.find(var_name);
            if (it == shader.buffer_meta_map.end())
            {
                BufferMeta buffer_meta;
                buffer_meta.set            = set;
                buffer_meta.binding        = binding;
                buffer_meta.size           = 0;
                buffer_meta.stageFlags     = stageFlags;
                buffer_meta.descriptorType = set_layout_binding.descriptorType;
                shader.buffer_meta_map.emplace(var_name, buffer_meta);
            }
            else
            {
                it->second.stageFlags |= stageFlags;
            }
        }
    }

    void ShaderFactory::GetStorageImagesMeta(Shader&                       shader,
                                             spirv_cross::Compiler&        compiler,
                                             spirv_cross::ShaderResources& resources,
                                             vk::ShaderStageFlags          stageFlags)
    {
        for (size_t i = 0; i < resources.storage_images.size(); ++i)
        {
            spirv_cross::Resource& res      = resources.storage_images[i];
            const std::string&     var_name = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding set_layout_binding(
                binding, vk::DescriptorType::eStorageImage, 1, stageFlags, nullptr);

            shader.set_layout_metas.AddDescriptorSetLayoutBinding(var_name, set, set_layout_binding);

            auto it = shader.image_meta_map.find(var_name);
            if (it == shader.image_meta_map.end())
            {
                ImageMeta image_meta;
                image_meta.set            = set;
                image_meta.binding        = binding;
                image_meta.stageFlags     = stageFlags;
                image_meta.descriptorType = set_layout_binding.descriptorType;
                shader.image_meta_map.emplace(var_name, image_meta);
            }
            else
            {
                it->second.stageFlags |= stageFlags;
            }
        }
    }

    void ShaderFactory::GenerateInputInfo(Shader& shader)
    {
        std::sort(shader.vertex_attribute_metas.begin(),
                  shader.vertex_attribute_metas.end(),
                  [](const VertexAttributeMeta& a, const VertexAttributeMeta& b) { return a.location < b.location; });

        shader.per_vertex_attributes.clear();
        shader.instance_attributes.clear();

        for (const auto& meta : shader.vertex_attribute_metas)
        {
            VertexAttributeBit attribute = meta.attribute;
            if (attribute == VertexAttributeBit::InstanceFloat1 || attribute == VertexAttributeBit::InstanceFloat2 ||
                attribute == VertexAttributeBit::InstanceFloat3 || attribute == VertexAttributeBit::InstanceFloat4)
            {
                shader.instance_attributes.push_back(attribute);
            }
            else
            {
                shader.per_vertex_attributes.push_back(attribute);
            }
        }

        shader.input_bindings.clear();
        if (!shader.per_vertex_attributes.empty())
        {
            uint32_t                          stride = VertexAttributesToSize(shader.per_vertex_attributes);
            vk::VertexInputBindingDescription per_vertex_input_binding(0, stride, vk::VertexInputRate::eVertex);
            shader.input_bindings.push_back(per_vertex_input_binding);
        }

        if (!shader.instance_attributes.empty())
        {
            uint32_t                          stride = VertexAttributesToSize(shader.instance_attributes);
            vk::VertexInputBindingDescription instanceInputBinding(1, stride, vk::VertexInputRate::eInstance);
            shader.input_bindings.push_back(instanceInputBinding);
        }

        uint32_t location = 0;
        shader.input_attributes.clear();

        if (!shader.per_vertex_attributes.empty())
        {
            uint32_t offset = 0;
            for (size_t i = 0; i < shader.per_vertex_attributes.size(); ++i)
            {
                vk::VertexInputAttributeDescription input_attribute(
                    0, location, VertexAttributeToVkFormat(shader.per_vertex_attributes[i]), offset);
                offset += VertexAttributeToSize(shader.per_vertex_attributes[i]);
                shader.input_attributes.push_back(input_attribute);
                location += 1;
            }
        }

        if (!shader.instance_attributes.empty())
        {
            uint32_t offset = 0;
            for (size_t i = 0; i < shader.instance_attributes.size(); ++i)
            {
                vk::VertexInputAttributeDescription input_attribute(
                    1, location, VertexAttributeToVkFormat(shader.instance_attributes[i]), offset);
                offset += VertexAttributeToSize(shader.instance_attributes[i]);
                shader.input_attributes.push_back(input_attribute);
                location += 1;
            }
        }
    }

    void ShaderFactory::GeneratePipelineLayout(Shader& shader)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        std::vector<DescriptorSetLayoutMeta>& metas = shader.set_layout_metas.metas;

        std::sort(metas.begin(), metas.end(), [](const DescriptorSetLayoutMeta& a, const DescriptorSetLayoutMeta& b) {
            return a.set < b.set;
        });

        for (auto& meta : metas)
        {
            std::sort(meta.bindings.begin(),
                      meta.bindings.end(),
                      [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) {
                          return a.binding < b.binding;
                      });
        }

        shader.descriptor_set_layouts.clear();
        if (metas.empty())
        {
            vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, 0, nullptr);
            shader.pipeline_layout = vk::raii::PipelineLayout(logical_device, pipeline_layout_create_info);
        }
        else
        {
            for (auto& set_layout_meta : metas)
            {
                vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(
                    vk::DescriptorSetLayoutCreateFlags(), set_layout_meta.bindings);

                vk::DescriptorSetLayout setLayout;
                logical_device.getDispatcher()->vkCreateDescriptorSetLayout(
                    static_cast<VkDevice>(*logical_device),
                    reinterpret_cast<const VkDescriptorSetLayoutCreateInfo*>(&descriptor_set_layout_create_info),
                    nullptr,
                    reinterpret_cast<VkDescriptorSetLayout*>(&setLayout));

                shader.descriptor_set_layouts.push_back(setLayout);
            }

            vk::PipelineLayoutCreateInfo pipeline_layout_create_info(
                {}, static_cast<uint32_t>(shader.descriptor_set_layouts.size()), shader.descriptor_set_layouts.data());
            shader.pipeline_layout = vk::raii::PipelineLayout(logical_device, pipeline_layout_create_info);
        }
    }

    void ShaderFactory::GenerateDynamicUniformBufferOffset(Shader& shader)
    {
        std::vector<DescriptorSetLayoutMeta>& metas = shader.set_layout_metas.metas;
        shader.dynamic_uniform_buffer_count         = 0;

        for (auto& meta : metas)
        {
            for (auto& descriptor_layout_binding : meta.bindings)
            {
                if (descriptor_layout_binding.descriptorType == vk::DescriptorType::eUniformBufferDynamic)
                {
                    for (auto& buffer_meta : shader.buffer_meta_map)
                    {
                        if (buffer_meta.second.set == meta.set &&
                            buffer_meta.second.binding == descriptor_layout_binding.binding &&
                            buffer_meta.second.descriptorType == descriptor_layout_binding.descriptorType &&
                            buffer_meta.second.stageFlags == descriptor_layout_binding.stageFlags)
                        {
                            buffer_meta.second.dynamic_seq = shader.dynamic_uniform_buffer_count;
                            shader.dynamic_uniform_buffer_count++;
                            break;
                        }
                    }
                }
            }
        }
    }
} // namespace Meow