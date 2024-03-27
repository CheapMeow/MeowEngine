#include "shader.h"

#include "core/log/log.h"
#include "function/global/runtime_global_context.h"
#include "function/render/structs/vertex_attribute.h"
#include "function/render/utils/vulkan_update_utils.h"

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
        for (int32_t i = 0; i < perVertexAttributes.size(); i++)
        {
            vertex_input_attribute_format_offset.emplace_back(VertexAttributeToVkFormat(perVertexAttributes[i]),
                                                              curr_offset);
            curr_offset += VertexAttributeToSize(perVertexAttributes[i]);
        }

        graphics_pipeline = MakeGraphicsPipeline(logical_device,
                                                 pipeline_cache,
                                                 vert_shader_module,
                                                 nullptr,
                                                 frag_shader_module,
                                                 nullptr,
                                                 VertexAttributesToSize(perVertexAttributes),
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
            const std::string&     varName   = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding setLayoutBinding {
                binding, vk::DescriptorType::eInputAttachment, 1, stageFlags, nullptr};

            set_layouts_meta.AddDescriptorSetLayoutBinding(varName, set, setLayoutBinding);

            // store mapping from image name to ImageInfo
            auto it = imageParams.find(varName);
            if (it == imageParams.end())
            {
                ImageInfo imageInfo      = {};
                imageInfo.set            = set;
                imageInfo.binding        = binding;
                imageInfo.stageFlags     = stageFlags;
                imageInfo.descriptorType = setLayoutBinding.descriptorType;
                imageParams.insert(std::make_pair(varName, imageInfo));
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
            const std::string&     varName                 = compiler.get_name(res.id);
            const std::string&     typeName                = compiler.get_name(res.base_type_id);
            uint32_t               uniformBufferStructSize = (uint32_t)compiler.get_declared_struct_size(type);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            // Convention: If expecting to use dynamic uniform buffer,
            // uniform name in shader should contain "Dynamic", for example:
            // [layout (binding = 0) uniform MVPDynamicBlock]
            vk::DescriptorSetLayoutBinding setLayoutBinding {
                binding,
                (typeName.find("Dynamic") != std::string::npos || use_dynamic_uniform_buffer) ?
                    vk::DescriptorType::eUniformBufferDynamic :
                    vk::DescriptorType::eUniformBuffer,
                1,
                stageFlags,
                nullptr};

            set_layouts_meta.AddDescriptorSetLayoutBinding(varName, set, setLayoutBinding);

            // store mapping from uniform buffer name to BufferInfo
            auto it = bufferParams.find(varName);
            if (it == bufferParams.end())
            {
                BufferInfo bufferInfo     = {};
                bufferInfo.set            = set;
                bufferInfo.binding        = binding;
                bufferInfo.bufferSize     = uniformBufferStructSize;
                bufferInfo.stageFlags     = stageFlags;
                bufferInfo.descriptorType = setLayoutBinding.descriptorType;
                bufferParams.insert(std::make_pair(varName, bufferInfo));
            }
            else
            {
                it->second.stageFlags |= setLayoutBinding.stageFlags;
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
            const std::string&     varName   = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding setLayoutBinding {
                binding, vk::DescriptorType::eCombinedImageSampler, 1, stageFlags, nullptr};

            set_layouts_meta.AddDescriptorSetLayoutBinding(varName, set, setLayoutBinding);

            auto it = imageParams.find(varName);
            if (it == imageParams.end())
            {
                ImageInfo imageInfo      = {};
                imageInfo.set            = set;
                imageInfo.binding        = binding;
                imageInfo.stageFlags     = stageFlags;
                imageInfo.descriptorType = setLayoutBinding.descriptorType;
                imageParams.insert(std::make_pair(varName, imageInfo));
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
            spirv_cross::Resource& res                = resources.stage_inputs[i];
            spirv_cross::SPIRType  type               = compiler.get_type(res.type_id);
            const std::string&     varName            = compiler.get_name(res.id);
            int32_t                inputAttributeSize = type.vecsize;

            // Convection: input vertex name should be certain name, for example:
            // inPosition, inUV0, ...
            VertexAttribute attribute = StringToVertexAttribute(varName.c_str());
            if (attribute == VertexAttribute::VA_None)
            {
                if (inputAttributeSize == 1)
                {
                    attribute = VertexAttribute::VA_InstanceFloat1;
                }
                else if (inputAttributeSize == 2)
                {
                    attribute = VertexAttribute::VA_InstanceFloat2;
                }
                else if (inputAttributeSize == 3)
                {
                    attribute = VertexAttribute::VA_InstanceFloat3;
                }
                else if (inputAttributeSize == 4)
                {
                    attribute = VertexAttribute::VA_InstanceFloat4;
                }
                // EDITOR_ERROR("Not found attribute : %s, treat as instance attribute : %d.", varName.c_str(),
                // int32(attribute));
            }

            // store tuple of input attribute and its location
            // location must be continous
            int32_t       location      = compiler.get_decoration(res.id, spv::DecorationLocation);
            AttributeInfo attributeInfo = {};
            attributeInfo.location      = location;
            attributeInfo.attribute     = attribute;
            attributeParams.push_back(attributeInfo);
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
            const std::string&     varName   = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding setLayoutBinding = {
                binding, vk::DescriptorType::eStorageBuffer, 1, stageFlags, nullptr};

            set_layouts_meta.AddDescriptorSetLayoutBinding(varName, set, setLayoutBinding);

            // store mapping from storage buffer name to BufferInfo
            auto it = bufferParams.find(varName);
            if (it == bufferParams.end())
            {
                BufferInfo bufferInfo     = {};
                bufferInfo.set            = set;
                bufferInfo.binding        = binding;
                bufferInfo.bufferSize     = 0;
                bufferInfo.stageFlags     = stageFlags;
                bufferInfo.descriptorType = setLayoutBinding.descriptorType;
                bufferParams.insert(std::make_pair(varName, bufferInfo));
            }
            else
            {
                it->second.stageFlags = it->second.stageFlags | setLayoutBinding.stageFlags;
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
            const std::string&     varName   = compiler.get_name(res.id);

            uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
            uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);

            vk::DescriptorSetLayoutBinding setLayoutBinding = {
                binding, vk::DescriptorType::eStorageImage, 1, stageFlags, nullptr};

            set_layouts_meta.AddDescriptorSetLayoutBinding(varName, set, setLayoutBinding);

            // store mapping from storage buffer name to BufferInfo
            auto it = imageParams.find(varName);
            if (it == imageParams.end())
            {
                ImageInfo imageInfo      = {};
                imageInfo.set            = set;
                imageInfo.binding        = binding;
                imageInfo.stageFlags     = stageFlags;
                imageInfo.descriptorType = setLayoutBinding.descriptorType;
                imageParams.insert(std::make_pair(varName, imageInfo));
            }
            else
            {
                it->second.stageFlags |= stageFlags;
            }
        }
    }

    void Shader::GenerateInputInfo()
    {
        // sort inputAttributes according to location
        std::sort(attributeParams.begin(),
                  attributeParams.end(),
                  [](const AttributeInfo& a, const AttributeInfo& b) -> bool { return a.location < b.location; });

        // sort inputAttributes to perVertexAttributes and instancesAttributes
        for (int32_t i = 0; i < attributeParams.size(); ++i)
        {
            VertexAttribute attribute = attributeParams[i].attribute;
            if (attribute == VA_InstanceFloat1 || attribute == VA_InstanceFloat2 || attribute == VA_InstanceFloat3 ||
                attribute == VA_InstanceFloat4)
            {
                instancesAttributes.push_back(attribute);
            }
            else
            {
                perVertexAttributes.push_back(attribute);
            }
        }

        // generate Bindinfo
        // first is perVertexInputBinding
        // second is instanceInputBinding
        inputBindings.resize(0);
        if (perVertexAttributes.size() > 0)
        {
            uint32_t stride = 0;
            for (int32_t i = 0; i < perVertexAttributes.size(); ++i)
            {
                stride += VertexAttributeToSize(perVertexAttributes[i]);
            }
            vk::VertexInputBindingDescription perVertexInputBinding {0, stride, vk::VertexInputRate::eVertex};
            inputBindings.push_back(perVertexInputBinding);
        }

        if (instancesAttributes.size() > 0)
        {
            uint32_t stride = 0;
            for (int32_t i = 0; i < instancesAttributes.size(); ++i)
            {
                stride += VertexAttributeToSize(instancesAttributes[i]);
            }
            vk::VertexInputBindingDescription instanceInputBinding {1, stride, vk::VertexInputRate::eInstance};
            inputBindings.push_back(instanceInputBinding);
        }

        // generate attributes info
        // first is perVertexAttributes
        // second is instancesAttributes
        uint32_t location = 0;
        if (perVertexAttributes.size() > 0)
        {
            uint32_t offset = 0;
            for (int32_t i = 0; i < perVertexAttributes.size(); ++i)
            {
                vk::VertexInputAttributeDescription inputAttribute {
                    0, location, VertexAttributeToVkFormat(perVertexAttributes[i]), offset};
                offset += VertexAttributeToSize(perVertexAttributes[i]);
                inputAttributes.push_back(inputAttribute);

                location += 1;
            }
        }

        if (instancesAttributes.size() > 0)
        {
            uint32_t offset = 0;
            for (int32_t i = 0; i < instancesAttributes.size(); ++i)
            {
                vk::VertexInputAttributeDescription inputAttribute {
                    1, location, VertexAttributeToVkFormat(instancesAttributes[i]), offset};
                offset += VertexAttributeToSize(instancesAttributes[i]);
                inputAttributes.push_back(inputAttribute);

                location += 1;
            }
        }
    }

    void Shader::GenerateLayout(vk::raii::Device const& raii_logical_device)
    {
        std::vector<DescriptorSetLayoutMeta>& setLayouts = set_layouts_meta.setLayouts;

        // first sort according to set
        std::sort(
            setLayouts.begin(),
            setLayouts.end(),
            [](const DescriptorSetLayoutMeta& a, const DescriptorSetLayoutMeta& b) -> bool { return a.set < b.set; });

        // first sort according to binding
        for (int32_t i = 0; i < setLayouts.size(); ++i)
        {
            std::vector<vk::DescriptorSetLayoutBinding>& bindings = setLayouts[i].bindings;
            std::sort(bindings.begin(),
                      bindings.end(),
                      [](const vk::DescriptorSetLayoutBinding& a, const vk::DescriptorSetLayoutBinding& b) -> bool {
                          return a.binding < b.binding;
                      });
        }

        // support multiple descriptor set layout
        for (int32_t i = 0; i < setLayouts.size(); ++i)
        {
            // DescriptorSetLayoutMeta& setLayoutInfo = setLayouts[i];

            // vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(vk::DescriptorSetLayoutCreateFlags
            // {},
            //                                                                     setLayoutInfo.bindings);
            // vk::DescriptorSetLayout           descriptor_set_layout =
            //     std::move(*raii_logical_device.createDescriptorSetLayout(descriptor_set_layout_create_info));

            DescriptorSetLayoutMeta& setLayoutInfo = setLayouts[i];

            vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info(vk::DescriptorSetLayoutCreateFlags {},
                                                                                setLayoutInfo.bindings);

            vk::DescriptorSetLayout setLayout;
            raii_logical_device.getDispatcher()->vkCreateDescriptorSetLayout(
                static_cast<VkDevice>(*raii_logical_device),
                reinterpret_cast<const VkDescriptorSetLayoutCreateInfo*>(&descriptor_set_layout_create_info),
                nullptr,
                reinterpret_cast<VkDescriptorSetLayout*>(&setLayout));

            descriptorSetLayouts.push_back(setLayout);
        }

        vk::PipelineLayoutCreateInfo pipeline_layout_create_info(
            {}, (uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data());
        pipeline_layout = vk::raii::PipelineLayout(raii_logical_device, pipeline_layout_create_info);
    }

    void Shader::AllocateDescriptorSet(vk::raii::Device const&         logical_device,
                                       vk::raii::DescriptorPool const& descriptor_pool)
    {
        // TODO: allocate descriptor set from a dynamic allocator
        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(
            *descriptor_pool, descriptorSetLayouts.size(), descriptorSetLayouts.data());
        vk::raii::DescriptorSets raii_descriptor_sets(logical_device, descriptor_set_allocate_info);

        for (size_t i = 0; i < raii_descriptor_sets.size(); ++i)
        {
            descriptor_sets.push_back(*std::move(raii_descriptor_sets[i]));
        }
    }

    void Shader::PushBufferWrite(const std::string&          name,
                                 vk::raii::Buffer const&     buffer,
                                 vk::raii::BufferView const* raii_buffer_view)
    {
        auto it = set_layouts_meta.paramsMap.find(name);
        if (it == set_layouts_meta.paramsMap.end())
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
            descriptor_sets[bindInfo.set],                                      // dstSet
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
        auto it = set_layouts_meta.paramsMap.find(name);
        if (it == set_layouts_meta.paramsMap.end())
        {
            // TODO: support log format
            // EDITOR_ERROR("Failed write buffer, %s not found!", name.c_str());
            return;
        }

        auto bindInfo = it->second;

        descriptor_image_infos.emplace_back(
            *texture_data.sampler, *texture_data.image_data.image_view, vk::ImageLayout::eShaderReadOnlyOptimal);

        write_descriptor_sets.emplace_back(
            descriptor_sets[bindInfo.set],                                      // dstSet
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
        // TODO: temp {*descriptor_sets[0], *descriptor_sets[1]}
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0, descriptor_sets, nullptr);
    }
} // namespace Meow