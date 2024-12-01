#include "forward_lighting_pass.h"

#include "pch.h"

#include "core/math/math.h"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_context.h"
#include "function/render/structs/per_scene_data.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    ForwardLightingPass::ForwardLightingPass()
        : RenderPass()
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

    void ForwardLightingPass::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        auto forward_lighting_shader_ptr = std::make_shared<Shader>(physical_device,
                                                                    logical_device,
                                                                    "builtin/shaders/forward_lighting.vert.spv",
                                                                    "builtin/shaders/forward_lighting.frag.spv");

        m_forward_lighting_mat = Material(forward_lighting_shader_ptr);
        m_forward_lighting_mat.CreatePipeline(vk::FrontFace::eClockwise, true);

        g_runtime_context.render_system->SetVertexAttributes(forward_lighting_shader_ptr->per_vertex_attributes);

        {
            auto texture_ptr =
                std::make_shared<ImageData>(ImageData::CreateTexture("builtin/textures/pbr_sphere/albedo.png"));
            if (texture_ptr)
            {
                g_runtime_context.resource_system->Register(texture_ptr);
                m_forward_lighting_mat.BindImageToDescriptorSet("albedoMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr =
                std::make_shared<ImageData>(ImageData::CreateTexture("builtin/textures/pbr_sphere/normal.png"));
            if (texture_ptr)
            {
                g_runtime_context.resource_system->Register(texture_ptr);
                m_forward_lighting_mat.BindImageToDescriptorSet("normalMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr =
                std::make_shared<ImageData>(ImageData::CreateTexture("builtin/textures/pbr_sphere/metallic.png"));
            if (texture_ptr)
            {
                g_runtime_context.resource_system->Register(texture_ptr);
                m_forward_lighting_mat.BindImageToDescriptorSet("metallicMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr =
                std::make_shared<ImageData>(ImageData::CreateTexture("builtin/textures/pbr_sphere/roughness.png"));
            if (texture_ptr)
            {
                g_runtime_context.resource_system->Register(texture_ptr);
                m_forward_lighting_mat.BindImageToDescriptorSet("roughnessMap", *texture_ptr);
            }
        }

        {
            auto texture_ptr =
                std::make_shared<ImageData>(ImageData::CreateTexture("builtin/textures/pbr_sphere/ao.png"));
            if (texture_ptr)
            {
                g_runtime_context.resource_system->Register(texture_ptr);
                m_forward_lighting_mat.BindImageToDescriptorSet("aoMap", *texture_ptr);
            }
        }

        // skybox

        auto skybox_shader_ptr = std::make_shared<Shader>(
            physical_device, logical_device, "builtin/shaders/skybox.vert.spv", "builtin/shaders/skybox.frag.spv");

        m_skybox_mat = Material(skybox_shader_ptr);
        m_skybox_mat.CreatePipeline(vk::FrontFace::eClockwise, true);

        {
            auto texture_ptr = std::make_shared<ImageData>(ImageData::CreateCubemap({
                "builtin/textures/cubemap/output_skybox_posx.hdr",
                "builtin/textures/cubemap/output_skybox_negx.hdr",
                "builtin/textures/cubemap/output_skybox_posy.hdr",
                "builtin/textures/cubemap/output_skybox_negy.hdr",
                "builtin/textures/cubemap/output_skybox_posz.hdr",
                "builtin/textures/cubemap/output_skybox_negz.hdr",
            }));
            if (texture_ptr)
            {
                g_runtime_context.resource_system->Register(texture_ptr);
                m_skybox_mat.BindImageToDescriptorSet("environmentMap", *texture_ptr);
            }
        }
    }

    void ForwardLightingPass::UpdateUniformBuffer()
    {
        FUNCTION_TIMER();

        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

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

        m_forward_lighting_mat.PopulateUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));

        struct LightData
        {
            glm::vec4 pos[4] = {
                glm::vec4(-10.0f, 10.0f, 10.0f, 0.0f),
                glm::vec4(10.0f, 10.0f, 10.0f, 0.0f),
                glm::vec4(-10.0f, -10.0f, 10.0f, 0.0f),
                glm::vec4(10.0f, -10.0f, 10.0f, 0.0f),
            };
            glm::vec4 color[4] = {
                glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
                glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
                glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
                glm::vec4(300.0f, 300.0f, 300.0f, 0.0f),
            };
            glm::vec3 camPos;
        };

        LightData lights;
        lights.camPos = transfrom_comp_ptr->position;

        m_forward_lighting_mat.PopulateUniformBuffer("lights", &lights, sizeof(lights));

        // Update mesh uniform

        m_forward_lighting_mat.BeginPopulatingDynamicUniformBufferPerFrame();
        const auto& all_gameobjects_map = level_ptr->GetAllVisibles();
        for (const auto& kv : all_gameobjects_map)
        {
            std::shared_ptr<GameObject>           gameobject_ptr = kv.second.lock();
            std::shared_ptr<Transform3DComponent> transfrom_comp_ptr2 =
                gameobject_ptr->TryGetComponent<Transform3DComponent>("Transform3DComponent");
            std::shared_ptr<ModelComponent> model_ptr =
                gameobject_ptr->TryGetComponent<ModelComponent>("ModelComponent");

            if (!transfrom_comp_ptr2 || !model_ptr)
                continue;

#ifdef MEOW_DEBUG
            if (!gameobject_ptr)
                MEOW_ERROR("shared ptr is invalid!");
            if (!transfrom_comp_ptr2)
                MEOW_ERROR("shared ptr is invalid!");
            if (!model_ptr)
                MEOW_ERROR("shared ptr is invalid!");
#endif

            auto model = transfrom_comp_ptr2->GetTransform();

            for (uint32_t i = 0; i < model_ptr->model_ptr.lock()->meshes.size(); ++i)
            {
                m_forward_lighting_mat.BeginPopulatingDynamicUniformBufferPerObject();
                m_forward_lighting_mat.PopulateDynamicUniformBuffer("objData", &model, sizeof(model));
                m_forward_lighting_mat.EndPopulatingDynamicUniformBufferPerObject();
            }
        }
        m_forward_lighting_mat.EndPopulatingDynamicUniformBufferPerFrame();

        // skybox

        static glm::mat4 capture_views[] = {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

        m_skybox_mat.BeginPopulatingDynamicUniformBufferPerFrame();
        per_scene_data.projection = Math::perspective_vk(
            glm::radians(90.0f), static_cast<float>(window_size[0]) / static_cast<float>(window_size[1]), 0.1f, 10.0f);
        for (uint32_t i = 0; i < 6; ++i)
        {
            m_skybox_mat.BeginPopulatingDynamicUniformBufferPerObject();
            per_scene_data.view = capture_views[i];
            m_skybox_mat.PopulateDynamicUniformBuffer("sceneData", &per_scene_data, sizeof(per_scene_data));
            m_skybox_mat.EndPopulatingDynamicUniformBufferPerObject();
        }
        m_skybox_mat.EndPopulatingDynamicUniformBufferPerFrame();
    }

    void ForwardLightingPass::BeforeRender(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

#ifdef MEOW_EDITOR
        command_buffer.resetQueryPool(*m_query_pool, 0, 1);
#endif
    }

    void ForwardLightingPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_forward_lighting_mat.BindPipeline(command_buffer);

#ifdef MEOW_EDITOR
        command_buffer.beginQuery(*m_query_pool, 0, {});
#endif

        m_forward_lighting_mat.BindDescriptorSetToPipeline(command_buffer, 0, 1);
        m_forward_lighting_mat.BindDescriptorSetToPipeline(command_buffer, 3, 1);

        std::shared_ptr<Level> level_ptr           = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        const auto&            all_gameobjects_map = level_ptr->GetAllVisibles();
        for (const auto& kv : all_gameobjects_map)
        {
            std::shared_ptr<GameObject> gameobject_ptr = kv.second.lock();
            if (!gameobject_ptr)
                continue;

            std::shared_ptr<ModelComponent> model_ptr =
                gameobject_ptr->TryGetComponent<ModelComponent>("ModelComponent");
            if (!model_ptr)
                continue;

            auto model_res_ptr = model_ptr->model_ptr.lock();
            if (!model_res_ptr)
                continue;

            m_forward_lighting_mat.BindDescriptorSetToPipeline(command_buffer, 1, 1);

            for (uint32_t i = 0; i < model_res_ptr->meshes.size(); ++i)
            {
                m_forward_lighting_mat.BindDescriptorSetToPipeline(command_buffer, 2, 1, true);
                model_res_ptr->meshes[i]->BindDrawCmd(command_buffer);
            }
        }

#ifdef MEOW_EDITOR
        command_buffer.endQuery(*m_query_pool, 0);
#endif
    }

    void ForwardLightingPass::AfterPresent()
    {
        FUNCTION_TIMER();

#ifdef MEOW_DEBUG
        std::pair<vk::Result, std::vector<uint32_t>> query_results =
            m_query_pool.getResults<uint32_t>(0, 1, sizeof(uint32_t) * 11, sizeof(uint32_t) * 11, {});

        g_runtime_context.profile_system->UploadPipelineStat(m_pass_name, query_results.second);

        g_runtime_context.profile_system->UploadMaterialStat(m_pass_name, m_forward_lighting_mat.GetStat());
#endif
    }

    void swap(ForwardLightingPass& lhs, ForwardLightingPass& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPass&>(lhs), static_cast<RenderPass&>(rhs));

        swap(lhs.m_forward_lighting_mat, rhs.m_forward_lighting_mat);
        swap(lhs.m_skybox_mat, rhs.m_skybox_mat);
    }
} // namespace Meow
