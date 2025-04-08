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
            Shader&                                         shader,
            vk::raii::ShaderModule&                         shader_module,
            const std::string&                              shader_file_path,
            vk::ShaderStageFlagBits                         stage,
            std::vector<vk::PipelineShaderStageCreateInfo>& pipeline_shader_stage_create_infos);

        void GetAttachmentsMeta(Shader&                       shader,
                                spirv_cross::Compiler&        compiler,
                                spirv_cross::ShaderResources& resources,
                                vk::ShaderStageFlags          stageFlags);

        void GetUniformBuffersMeta(Shader&                       shader,
                                   spirv_cross::Compiler&        compiler,
                                   spirv_cross::ShaderResources& resources,
                                   vk::ShaderStageFlags          stageFlags);

        void GetTexturesMeta(Shader&                       shader,
                             spirv_cross::Compiler&        compiler,
                             spirv_cross::ShaderResources& resources,
                             vk::ShaderStageFlags          stageFlags);

        void GetInputMeta(Shader&                       shader,
                          spirv_cross::Compiler&        compiler,
                          spirv_cross::ShaderResources& resources,
                          vk::ShaderStageFlags          stageFlags);

        void GetStorageBuffersMeta(Shader&                       shader,
                                   spirv_cross::Compiler&        compiler,
                                   spirv_cross::ShaderResources& resources,
                                   vk::ShaderStageFlags          stageFlags);

        void GetStorageImagesMeta(Shader&                       shader,
                                  spirv_cross::Compiler&        compiler,
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
    };
} // namespace Meow