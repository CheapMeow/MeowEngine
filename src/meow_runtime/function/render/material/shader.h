#pragma once

#include "pch.h"

#include "function/render/buffer_data/image_data.h"
#include "function/render/model/vertex_attribute.h"

#include <spirv_cross/spirv_glsl.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <iostream>
#include <list>
#include <unordered_map>

namespace Meow
{
    struct VertexAttributeMeta
    {
        VertexAttributeBit attribute;
        int32_t            location;
    };

    struct BufferMeta
    {
        uint32_t             set            = 0;
        uint32_t             binding        = 0;
        uint32_t             size           = 0;
        vk::DescriptorType   descriptorType = vk::DescriptorType::eUniformBuffer;
        vk::ShaderStageFlags stageFlags     = {};

        /**
         * This value only available when dynamic uniform buffer.
         *
         * Vulkan Specification: If any of the sets being bound include dynamic uniform or storage buffers, then
         * pDynamicOffsets includes one element for each array element in each dynamic descriptor type binding in each
         * set. Values are taken from pDynamicOffsets in an order such that all entries for set N come before set N+1;
         * within a set, entries are ordered by the binding numbers in the descriptor set layouts; and within a binding
         * array, elements are in order. dynamicOffsetCount must equal the total number of dynamic descriptors in the
         * sets being bound.
         */
        uint32_t dynamic_seq = 0;

#ifdef MEOW_DEBUG
        std::string var_name;
        std::string type_name;
#endif
    };

    struct ImageMeta
    {
        uint32_t             set            = 0;
        uint32_t             binding        = 0;
        vk::DescriptorType   descriptorType = vk::DescriptorType::eCombinedImageSampler;
        vk::ShaderStageFlags stageFlags     = {};
    };

    class DescriptorSetLayoutMeta
    {
        using BindingsArray = std::vector<vk::DescriptorSetLayoutBinding>;

    public:
        DescriptorSetLayoutMeta() {}

        ~DescriptorSetLayoutMeta() {}

        uint32_t      set = -1;
        BindingsArray bindings;
    };

    class DescriptorSetLayoutMetas
    {
    public:
        struct BindingMeta
        {
            int32_t set;
            int32_t binding;
        };

        DescriptorSetLayoutMetas() {}

        ~DescriptorSetLayoutMetas() {}

        vk::DescriptorType GetDescriptorType(int32_t set, int32_t binding)
        {
            for (int32_t i = 0; i < metas.size(); ++i)
            {
                if (metas[i].set == set)
                {
                    for (int32_t j = 0; j < metas[i].bindings.size(); ++j)
                    {
                        if (metas[i].bindings[j].binding == binding)
                        {
                            return metas[i].bindings[j].descriptorType;
                        }
                    }
                }
            }

            // There is not vk::DescriptorType correspond to VK_DESCRIPTOR_TYPE_MAX_ENUM?
            return vk::DescriptorType::eMutableEXT;
        }

        void
        AddDescriptorSetLayoutBinding(const std::string& varName, int32_t set, vk::DescriptorSetLayoutBinding binding)
        {
            DescriptorSetLayoutMeta* setLayout = nullptr;

            // find existing set layout
            // this supports multiple set
            for (int32_t i = 0; i < metas.size(); ++i)
            {
                if (metas[i].set == set)
                {
                    setLayout = &(metas[i]);
                    break;
                }
            }

            // If there is not set layout, new one
            if (setLayout == nullptr)
            {
                metas.push_back({});
                setLayout = &(metas[metas.size() - 1]);
            }

            for (int32_t i = 0; i < setLayout->bindings.size(); ++i)
            {
                vk::DescriptorSetLayoutBinding& setBinding = setLayout->bindings[i];
                if (setBinding.binding == binding.binding && setBinding.descriptorType == binding.descriptorType)
                {
                    setBinding.stageFlags = setBinding.stageFlags | binding.stageFlags;
                    return;
                }
            }

            setLayout->set = set;
            setLayout->bindings.push_back(binding);

            // save mapping from parameter name to set and binding
            BindingMeta paramInfo = {};
            paramInfo.set         = set;
            paramInfo.binding     = binding.binding;
            binding_meta_map.insert(std::make_pair(varName, paramInfo));
        }

        std::unordered_map<std::string, BindingMeta> binding_meta_map;
        std::vector<DescriptorSetLayoutMeta>         metas;
    };

    /**
     * @brief Always use dynamic uniform buffer. It may lead to memory waste. For example, one debug material only need
     * one parameter block uniform buffer, but the Material class still allocate 32KB ring buffer for it.
     * TODO: support un-dynamic
     */
    struct Shader
    {
        using InputBindingsVector   = std::vector<vk::VertexInputBindingDescription>;
        using InputAttributesVector = std::vector<vk::VertexInputAttributeDescription>;

        // stored for creating pipeline

        vk::raii::ShaderModule vert_shader_module = nullptr;
        vk::raii::ShaderModule frag_shader_module = nullptr;
        vk::raii::ShaderModule geom_shader_module = nullptr;
        vk::raii::ShaderModule comp_shader_module = nullptr;
        vk::raii::ShaderModule tesc_shader_module = nullptr;
        vk::raii::ShaderModule tese_shader_module = nullptr;

        // a dirty hack
        // because raii class doesn't provide operator== override

        bool is_vert_shader_valid = false;
        bool is_frag_shader_valid = false;
        bool is_geom_shader_valid = false;
        bool is_comp_shader_valid = false;
        bool is_tesc_shader_valid = false;
        bool is_tese_shader_valid = false;

        DescriptorSetLayoutMetas set_layout_metas;

        std::vector<VertexAttributeMeta>            vertex_attribute_metas;
        std::unordered_map<std::string, BufferMeta> buffer_meta_map;
        std::unordered_map<std::string, ImageMeta>  image_meta_map;

        uint32_t dynamic_uniform_buffer_count = 0;

        std::vector<VertexAttributeBit> per_vertex_attributes;
        std::vector<VertexAttributeBit> instance_attributes;

        InputBindingsVector   input_bindings;
        InputAttributesVector input_attributes;

        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;

        vk::raii::PipelineLayout pipeline_layout = nullptr;

    private:
        vk::Device                        m_device     = {};
        const vk::raii::DeviceDispatcher* m_dispatcher = nullptr;

    public:
        Shader() {}

        Shader(const vk::raii::PhysicalDevice& physical_device,
               const vk::raii::Device&         logical_device,
               std::string                     vert_shader_file_path,
               std::string                     frag_shader_file_path,
               std::string                     geom_shader_file_path = "",
               std::string                     comp_shader_file_path = "",
               std::string                     tesc_shader_file_path = "",
               std::string                     tese_shader_file_path = "");

        ~Shader()
        {
            for (int i = 0; i < descriptor_set_layouts.size(); ++i)
            {
                m_dispatcher->vkDestroyDescriptorSetLayout(
                    static_cast<VkDevice>(m_device),
                    static_cast<VkDescriptorSetLayout>(descriptor_set_layouts[i]),
                    nullptr);
            }
        }

    private:
        bool CreateShaderModuleAndGetMeta(
            const vk::raii::Device&                         logical_device,
            vk::raii::ShaderModule&                         shader_module,
            const std::string&                              shader_file_path,
            vk::ShaderStageFlagBits                         stage,
            std::vector<vk::PipelineShaderStageCreateInfo>& pipeline_shader_stage_create_infos);

        void GetAttachmentsMeta(spirv_cross::Compiler&        compiler,
                                spirv_cross::ShaderResources& resources,
                                vk::ShaderStageFlags          stageFlags);

        void GetUniformBuffersMeta(spirv_cross::Compiler&        compiler,
                                   spirv_cross::ShaderResources& resources,
                                   vk::ShaderStageFlags          stageFlags);

        void GetTexturesMeta(spirv_cross::Compiler&        compiler,
                             spirv_cross::ShaderResources& resources,
                             vk::ShaderStageFlags          stageFlags);

        void GetInputMeta(spirv_cross::Compiler&        compiler,
                          spirv_cross::ShaderResources& resources,
                          vk::ShaderStageFlags          stageFlags);

        void GetStorageBuffersMeta(spirv_cross::Compiler&        compiler,
                                   spirv_cross::ShaderResources& resources,
                                   vk::ShaderStageFlags          stageFlags);

        void GetStorageImagesMeta(spirv_cross::Compiler&        compiler,
                                  spirv_cross::ShaderResources& resources,
                                  vk::ShaderStageFlags          stageFlags);

        void GenerateInputInfo();

        void GeneratePipelineLayout(const vk::raii::Device& raii_logical_device);

        void GenerateDynamicUniformBufferOffset();
    };

} // namespace Meow
