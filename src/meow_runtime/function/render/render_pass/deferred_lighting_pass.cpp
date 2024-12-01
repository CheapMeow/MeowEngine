#include "deferred_lighting_pass.h"

#include "pch.h"

#include "core/math/math.h"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_context.h"
#include "function/object/game_object.h"
#include "function/render/structs/per_scene_data.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

namespace Meow
{
    DeferredLightingPass::DeferredLightingPass()
    {
        CreateMaterial();

#ifdef MEOW_EDITOR
        m_pass_name = "Deferred lighting Pass";

        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        VkQueryPoolCreateInfo m_query_pool_create_info = {.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                                          .queryType  = VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                                          .queryCount = 1,
                                                          .pipelineStatistics = (1 << 11) - 1};

        m_query_pool = logical_device.createQueryPool(m_query_pool_create_info, nullptr);
#endif
    }

    void DeferredLightingPass::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        auto quad_shader_ptr = std::make_shared<Shader>(physical_device,
                                                        logical_device,
                                                        "builtin/shaders/deferred_lighting.vert.spv",
                                                        "builtin/shaders/deferred_lighting.frag.spv");

        m_deferred_lighting_mat         = Material(quad_shader_ptr);
        m_deferred_lighting_mat.subpass = 1;
        m_deferred_lighting_mat.CreatePipeline(vk::FrontFace::eClockwise, false);

        // Create quad model
        std::vector<float>    vertices = {-1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
                                          1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f};
        std::vector<uint32_t> indices  = {0, 1, 2, 0, 2, 3};

        m_quad_model = std::move(
            Model(std::move(vertices), std::move(indices), m_deferred_lighting_mat.shader_ptr->per_vertex_attributes));

        for (int32_t i = 0; i < k_num_lights; ++i)
        {
            m_LightDatas.lights[i].position.x = glm::linearRand<float>(-10.0f, 10.0f);
            m_LightDatas.lights[i].position.y = glm::linearRand<float>(-10.0f, 10.0f);
            m_LightDatas.lights[i].position.z = glm::linearRand<float>(-10.0f, 10.0f);

            m_LightDatas.lights[i].color.x = glm::linearRand<float>(0.0f, 1.0f);
            m_LightDatas.lights[i].color.y = glm::linearRand<float>(0.0f, 1.0f);
            m_LightDatas.lights[i].color.z = glm::linearRand<float>(0.0f, 1.0f);

            m_LightDatas.lights[i].radius = glm::linearRand<float>(0.0f, 5.0f);

            m_LightInfos.position[i]  = m_LightDatas.lights[i].position;
            m_LightInfos.direction[i] = m_LightInfos.position[i];
            m_LightInfos.direction[i] = normalize(m_LightInfos.direction[i]);
            m_LightInfos.speed[i]     = 1.0f + glm::linearRand<float>(1.0f, 2.0f);
        }
    }

    void DeferredLightingPass::RefreshAttachments(ImageData& color_attachment,
                                                  ImageData& normal_attachment,
                                                  ImageData& position_attachment,
                                                  ImageData& depth_attachment)
    {
        m_deferred_lighting_mat.BindImageToDescriptorSet("inputColor", color_attachment);
        m_deferred_lighting_mat.BindImageToDescriptorSet("inputNormal", normal_attachment);
        m_deferred_lighting_mat.BindImageToDescriptorSet("inputPosition", position_attachment);
        m_deferred_lighting_mat.BindImageToDescriptorSet("inputDepth", depth_attachment);
    }

    void DeferredLightingPass::UpdateUniformBuffer()
    {
        FUNCTION_TIMER();

        // update light

        for (int32_t i = 0; i < k_num_lights; ++i)
        {
            float bias = glm::sin(g_runtime_context.time_system->GetTime() * m_LightInfos.speed[i]) / 5.0f;
            m_LightDatas.lights[i].position.x = m_LightInfos.position[i].x + bias * m_LightInfos.direction[i].x;
            m_LightDatas.lights[i].position.y = m_LightInfos.position[i].y + bias * m_LightInfos.direction[i].y;
            m_LightDatas.lights[i].position.z = m_LightInfos.position[i].z + bias * m_LightInfos.direction[i].z;
        }

        m_deferred_lighting_mat.PopulateUniformBuffer("lightDatas", &m_LightDatas, sizeof(m_LightDatas));
    }

    void DeferredLightingPass::BeforeRender(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

#ifdef MEOW_EDITOR
        command_buffer.resetQueryPool(*m_query_pool, 0, 1);
#endif
    }

    void DeferredLightingPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_deferred_lighting_mat.BindDescriptorSetToPipeline(command_buffer, 0, 1);

#ifdef MEOW_EDITOR
        command_buffer.beginQuery(*m_query_pool, 0, {});
#endif

        for (int32_t i = 0; i < m_quad_model.meshes.size(); ++i)
        {
            m_quad_model.meshes[i]->BindDrawCmd(command_buffer);
        }

#ifdef MEOW_EDITOR
        command_buffer.endQuery(*m_query_pool, 0);
#endif
    }

    void DeferredLightingPass::AfterPresent()
    {
        FUNCTION_TIMER();

#ifdef MEOW_DEBUG
        std::pair<vk::Result, std::vector<uint32_t>> query_results =
            m_query_pool.getResults<uint32_t>(0, 1, sizeof(uint32_t) * 11, sizeof(uint32_t) * 11, {});

        g_runtime_context.profile_system->UploadPipelineStat(m_pass_name, query_results.second);

        g_runtime_context.profile_system->UploadMaterialStat(m_pass_name, m_deferred_lighting_mat.GetStat());
#endif
    }

    void swap(DeferredLightingPass& lhs, DeferredLightingPass& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPass&>(lhs), static_cast<RenderPass&>(rhs));

        swap(lhs.m_deferred_lighting_mat, rhs.m_deferred_lighting_mat);
        swap(lhs.m_quad_model, rhs.m_quad_model);

        swap(lhs.m_LightDatas, rhs.m_LightDatas);
        swap(lhs.m_LightInfos, rhs.m_LightInfos);
    }
} // namespace Meow