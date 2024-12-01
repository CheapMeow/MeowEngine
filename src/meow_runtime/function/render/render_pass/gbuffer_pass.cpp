#include "gbuffer_pass.h"

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
    GbufferPass::GbufferPass()
    {
        CreateMaterial();

#ifdef MEOW_EDITOR
        m_pass_name = "Gbuffer Pass";

        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        VkQueryPoolCreateInfo m_query_pool_create_info = {.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                                          .queryType  = VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                                          .queryCount = 1,
                                                          .pipelineStatistics = (1 << 11) - 1};

        m_query_pool = logical_device.createQueryPool(m_query_pool_create_info, nullptr);
#endif
    }

    void GbufferPass::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        auto gbuffer_shader_ptr = std::make_shared<Shader>(
            physical_device, logical_device, "builtin/shaders/gbuffer.vert.spv", "builtin/shaders/gbuffer.frag.spv");

        m_gbuffer_mat                        = Material(gbuffer_shader_ptr);
        m_gbuffer_mat.color_attachment_count = 3;
        m_gbuffer_mat.CreatePipeline(vk::FrontFace::eClockwise, true);

        g_runtime_context.render_system->SetVertexAttributes(gbuffer_shader_ptr->per_vertex_attributes);
    }

    void GbufferPass::UpdateUniformBuffer()
    {
        FUNCTION_TIMER();

        std::shared_ptr<Level> level_ptr = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

#ifdef MEOW_DEBUG
        if (!level_ptr)
            MEOW_ERROR("shared ptr is invalid!");
#endif

        std::shared_ptr<GameObject> camera_go_ptr = level_ptr->GetGameObjectByID(level_ptr->GetMainCameraID()).lock();
        std::shared_ptr<Transform3DComponent> transfrom_comp_ptr =
            camera_go_ptr->TryGetComponent<Transform3DComponent>("Transform3DComponent");
        std::shared_ptr<Camera3DComponent> camera_comp_ptr =
            camera_go_ptr->TryGetComponent<Camera3DComponent>("Camera3DComponent");

#ifdef MEOW_DEBUG
        if (!camera_go_ptr)
            MEOW_ERROR("shared ptr is invalid!");
        if (!transfrom_comp_ptr)
            MEOW_ERROR("shared ptr is invalid!");
        if (!camera_comp_ptr)
            MEOW_ERROR("shared ptr is invalid!");
#endif

        glm::ivec2 window_size = g_runtime_context.window_system->GetCurrentFocusWindow()->GetSize();

        glm::vec3 forward = transfrom_comp_ptr->rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        glm::mat4 view =
            lookAt(transfrom_comp_ptr->position, transfrom_comp_ptr->position + forward, glm::vec3(0.0f, 1.0f, 0.0f));

        PerSceneData per_scene_data;
        per_scene_data.view = view;
        per_scene_data.projection =
            Math::perspective_vk(camera_comp_ptr->field_of_view,
                                 static_cast<float>(window_size[0]) / static_cast<float>(window_size[1]),
                                 camera_comp_ptr->near_plane,
                                 camera_comp_ptr->far_plane);

        m_gbuffer_mat.PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));

        // Update mesh uniform

        m_gbuffer_mat.BeginPopulatingDynamicUniformBufferPerFrame();
        const auto& all_gameobjects_map = level_ptr->GetAllVisibles();
        for (const auto& kv : all_gameobjects_map)
        {
            std::shared_ptr<GameObject>           model_go_ptr = kv.second.lock();
            std::shared_ptr<Transform3DComponent> transfrom_comp_ptr2 =
                model_go_ptr->TryGetComponent<Transform3DComponent>("Transform3DComponent");
            std::shared_ptr<ModelComponent> model_comp_ptr =
                model_go_ptr->TryGetComponent<ModelComponent>("ModelComponent");

            if (!transfrom_comp_ptr2 || !model_comp_ptr)
                continue;

#ifdef MEOW_DEBUG
            if (!model_go_ptr)
                MEOW_ERROR("shared ptr is invalid!");
            if (!transfrom_comp_ptr2)
                MEOW_ERROR("shared ptr is invalid!");
            if (!model_comp_ptr)
                MEOW_ERROR("shared ptr is invalid!");
#endif

            auto model = transfrom_comp_ptr2->GetTransform();

            for (uint32_t i = 0; i < model_comp_ptr->model_ptr.lock()->meshes.size(); ++i)
            {
                m_gbuffer_mat.BeginPopulatingDynamicUniformBufferPerObject();
                m_gbuffer_mat.PopulateDynamicUniformBuffer("objData", &model, sizeof(model));
                m_gbuffer_mat.EndPopulatingDynamicUniformBufferPerObject();
            }
        }
        m_gbuffer_mat.EndPopulatingDynamicUniformBufferPerFrame();
    }

    void GbufferPass::BeforeRender(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

#ifdef MEOW_EDITOR
        command_buffer.resetQueryPool(*m_query_pool, 0, 1);
#endif
    }

    void GbufferPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_gbuffer_mat.BindDescriptorSetToPipeline(command_buffer, 0, 1);

#ifdef MEOW_EDITOR
        command_buffer.beginQuery(*m_query_pool, 0, {});
#endif

        std::shared_ptr<Level> level_ptr           = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        const auto&            all_gameobjects_map = level_ptr->GetAllVisibles();
        for (const auto& kv : all_gameobjects_map)
        {
            std::shared_ptr<GameObject> model_go_ptr = kv.second.lock();
            if (!model_go_ptr)
                continue;

            std::shared_ptr<ModelComponent> model_comp_ptr =
                model_go_ptr->TryGetComponent<ModelComponent>("ModelComponent");
            if (!model_comp_ptr)
                continue;

            auto model_res_ptr = model_comp_ptr->model_ptr.lock();
            if (!model_res_ptr)
                continue;

            for (uint32_t i = 0; i < model_res_ptr->meshes.size(); ++i)
            {
                m_gbuffer_mat.BindDescriptorSetToPipeline(command_buffer, 1, 1, true);
                model_res_ptr->meshes[i]->BindDrawCmd(command_buffer);
            }
        }

#ifdef MEOW_EDITOR
        command_buffer.endQuery(*m_query_pool, 0);
#endif
    }

    void GbufferPass::AfterPresent()
    {
        FUNCTION_TIMER();

#ifdef MEOW_DEBUG
        std::pair<vk::Result, std::vector<uint32_t>> query_results =
            m_query_pool.getResults<uint32_t>(0, 1, sizeof(uint32_t) * 11, sizeof(uint32_t) * 11, {});

        g_runtime_context.profile_system->UploadPipelineStat(m_pass_name, query_results.second);

        g_runtime_context.profile_system->UploadMaterialStat(m_pass_name, m_gbuffer_mat.GetStat());
#endif
    }

    void swap(GbufferPass& lhs, GbufferPass& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPass&>(lhs), static_cast<RenderPass&>(rhs));

        swap(lhs.m_gbuffer_mat, rhs.m_gbuffer_mat);
    }
} // namespace Meow