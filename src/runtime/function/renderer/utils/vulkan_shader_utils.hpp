#pragma once

#include "SPIRV/GlslangToSpv.h"

#include <vulkan/vulkan_raii.hpp>

#include <string>
#include <vector>

namespace vk
{
    namespace Meow
    {
        EShLanguage TranslateShaderStage(vk::ShaderStageFlagBits stage)
        {
            switch (stage)
            {
                case vk::ShaderStageFlagBits::eVertex:
                    return EShLangVertex;
                case vk::ShaderStageFlagBits::eTessellationControl:
                    return EShLangTessControl;
                case vk::ShaderStageFlagBits::eTessellationEvaluation:
                    return EShLangTessEvaluation;
                case vk::ShaderStageFlagBits::eGeometry:
                    return EShLangGeometry;
                case vk::ShaderStageFlagBits::eFragment:
                    return EShLangFragment;
                case vk::ShaderStageFlagBits::eCompute:
                    return EShLangCompute;
                case vk::ShaderStageFlagBits::eRaygenNV:
                    return EShLangRayGenNV;
                case vk::ShaderStageFlagBits::eAnyHitNV:
                    return EShLangAnyHitNV;
                case vk::ShaderStageFlagBits::eClosestHitNV:
                    return EShLangClosestHitNV;
                case vk::ShaderStageFlagBits::eMissNV:
                    return EShLangMissNV;
                case vk::ShaderStageFlagBits::eIntersectionNV:
                    return EShLangIntersectNV;
                case vk::ShaderStageFlagBits::eCallableNV:
                    return EShLangCallableNV;
                case vk::ShaderStageFlagBits::eTaskNV:
                    return EShLangTaskNV;
                case vk::ShaderStageFlagBits::eMeshNV:
                    return EShLangMeshNV;
                default:
                    assert(false && "Unknown shader stage");
                    return EShLangVertex;
            }
        }

        bool GLSLtoSPV(const vk::ShaderStageFlagBits shader_type,
                       std::string const&            glsl_shader,
                       std::vector<unsigned int>&    spv_shader)
        {
            EShLanguage stage = TranslateShaderStage(shader_type);

            const char* shaderStrings[1];
            shaderStrings[0] = glsl_shader.data();

            glslang::TShader shader(stage);
            shader.setStrings(shaderStrings, 1);

            // Enable SPIR-V and Vulkan rules when parsing GLSL
            EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

            if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages))
            {
                puts(shader.getInfoLog());
                puts(shader.getInfoDebugLog());
                return false; // something didn't work
            }

            glslang::TProgram program;
            program.addShader(&shader);

            //
            // Program-level processing...
            //

            if (!program.link(messages))
            {
                puts(shader.getInfoLog());
                puts(shader.getInfoDebugLog());
                fflush(stdout);
                return false;
            }

            glslang::GlslangToSpv(*program.getIntermediate(stage), spv_shader);
            return true;
        }

        template<typename Dispatcher = VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>
        vk::raii::ShaderModule MakeShaderModule(vk::raii::Device const& device,
                                                vk::ShaderStageFlagBits shader_stage,
                                                std::string const&      shader_text)
        {
            std::vector<unsigned int> shader_spv;
            if (!GLSLtoSPV(shader_stage, shader_text, shader_spv))
            {
                throw std::runtime_error("Could not convert glsl shader to spir-v -> terminating");
            }

            return vk::raii::ShaderModule(device,
                                          vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), shader_spv));
        }
    } // namespace Meow
} // namespace vk

// vertex shader with (P)osition and (C)olor in and (C)olor out
const std::string vertexShaderText_PC_C = R"(
#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 0) uniform buffer
{
  mat4 mvp;
} uniformBuffer;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main()
{
  outColor = inColor;
  gl_Position = uniformBuffer.mvp * pos;
}
)";

// vertex shader with (P)osition and (T)exCoord in and (T)exCoord out
const std::string vertexShaderText_PT_T = R"(
#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 0) uniform buffer
{
  mat4 mvp;
} uniformBuffer;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec2 outTexCoord;

void main()
{
  outTexCoord = inTexCoord;
  gl_Position = uniformBuffer.mvp * pos;
}
)";

// fragment shader with (C)olor in and (C)olor out
const std::string fragmentShaderText_C_C = R"(
#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 color;

layout (location = 0) out vec4 outColor;

void main()
{
  outColor = color;
}
)";

// fragment shader with (T)exCoord in and (C)olor out
const std::string fragmentShaderText_T_C = R"(
#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D tex;

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

void main()
{
  outColor = texture(tex, inTexCoord);
}
)";
