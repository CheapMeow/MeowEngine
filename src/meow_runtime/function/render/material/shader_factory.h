#pragma once

#include "shader.h"

namespace Meow
{
    class ShaderFactory
    {
    public:
        ShaderFactory()  = default;
        ~ShaderFactory() = default;

        ShaderFactory& clear();
        ShaderFactory& SetVertexShader(const std::string& vert_shader_file_path);
        ShaderFactory& SetFragmentShader(const std::string& frag_shader_file_path);
        ShaderFactory& SetGeometryShader(const std::string& geom_shader_file_path);
        ShaderFactory& SetComputeShader(const std::string& comp_shader_file_path);
        ShaderFactory& SetTessellationControlShader(const std::string& tesc_shader_file_path);
        ShaderFactory& SetTessellationEvaluationShader(const std::string& tese_shader_file_path);

        std::shared_ptr<Shader> Create();

    private:
        bool CreateShaderModuleAndGetMeta(
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

        void GenerateInputInfo(Shader& shader);
        void GeneratePipelineLayout(Shader& shader);
        void GenerateDynamicUniformBufferOffset(Shader& shader);

    private:
        std::string m_vert_shader_file_path;
        std::string m_frag_shader_file_path;
        std::string m_geom_shader_file_path;
        std::string m_comp_shader_file_path;
        std::string m_tesc_shader_file_path;
        std::string m_tese_shader_file_path;

        DescriptorSetLayoutMetas                    m_set_layout_metas;
        std::vector<VertexAttributeMeta>            m_vertex_attribute_metas;
        std::unordered_map<std::string, BufferMeta> m_buffer_meta_map;
        std::unordered_map<std::string, ImageMeta>  m_image_meta_map;
        std::vector<VertexAttributeBit>             m_per_vertex_attributes;
        std::vector<VertexAttributeBit>             m_instance_attributes;
    };
} // namespace Meow