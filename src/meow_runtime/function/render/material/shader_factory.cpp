#include "shader_factory.h"

namespace Meow
{
    void ShaderFactory::clear()
    {
        m_vert_shader_file_path.clear();
        m_frag_shader_file_path.clear();
        m_geom_shader_file_path.clear();
        m_comp_shader_file_path.clear();
        m_tesc_shader_file_path.clear();
        m_tese_shader_file_path.clear();
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
} // namespace Meow