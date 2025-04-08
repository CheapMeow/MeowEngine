#pragma once

#include <string>

namespace Meow
{
    class ShaderFactory
    {
    public:
        ShaderFactory() = default;

        ~ShaderFactory() = default;

        void           clear();
        ShaderFactory& SetVertexShader(const std::string& vert_shader_file_path);
        ShaderFactory& SetFragmentShader(const std::string& frag_shader_file_path);
        ShaderFactory& SetGeometryShader(const std::string& geom_shader_file_path);
        ShaderFactory& SetComputeShader(const std::string& comp_shader_file_path);
        ShaderFactory& SetTessellationControlShader(const std::string& tesc_shader_file_path);
        ShaderFactory& SetTessellationEvaluationShader(const std::string& tese_shader_file_path);

        
    private:
        std::string m_vert_shader_file_path;
        std::string m_frag_shader_file_path;
        std::string m_geom_shader_file_path;
        std::string m_comp_shader_file_path;
        std::string m_tesc_shader_file_path;
        std::string m_tese_shader_file_path;
    };
} // namespace Meow
