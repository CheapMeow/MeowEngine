#include "render_system.h"

#include "function/ecs/components/3d/camera/camera_3d_component.h"
#include "function/ecs/components/3d/model/model_component.h"
#include "function/ecs/components/3d/transform/transform_3d_component.h"
#include "function/ecs/components/shared/pawn_component.h"
#include "function/global/runtime_global_context.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    RenderSystem::RenderSystem()
    {
        g_runtime_global_context.renderer = std::make_shared<VulkanRenderer>();

        // TODO: creating entity should be obstract
        const auto camera_entity = g_runtime_global_context.registry.create();
        auto&      camera_transform_component =
            g_runtime_global_context.registry.emplace<Transform3DComponent>(camera_entity);
        auto& pawn_component            = g_runtime_global_context.registry.emplace<PawnComponent>(camera_entity);
        pawn_component.is_player        = true;
        auto& camera_component          = g_runtime_global_context.registry.emplace<Camera3DComponent>(camera_entity);
        camera_component.is_main_camera = true;

        const auto model_entity = g_runtime_global_context.registry.create();
        g_runtime_global_context.registry.emplace<Transform3DComponent>(model_entity);
        auto& model_component = g_runtime_global_context.registry.emplace<ModelComponent>(
            model_entity,
            "builtin/models/monkey_head.obj",
            g_runtime_global_context.renderer->m_gpu,
            g_runtime_global_context.renderer->m_logical_device,
            std::vector<VertexAttribute> {VertexAttribute::VA_Position, VertexAttribute::VA_Normal});

        BoundingBox model_bounding = model_component.model.GetBounding();
        glm::vec3   bound_size     = model_bounding.max - model_bounding.min;
        glm::vec3   bound_center   = model_bounding.min + bound_size * 0.5f;
        camera_transform_component.global_transform =
            glm::translate(glm::mat4(1.0f), glm::vec3(bound_center.x, bound_center.y, bound_center.z - 50.0f));
    }

    RenderSystem::~RenderSystem()
    {
        // TODO: deleting entity should be obstract
        auto model_view = g_runtime_global_context.registry.view<Transform3DComponent, ModelComponent>();
        g_runtime_global_context.registry.destroy(model_view.begin(), model_view.end());

        auto camera_view = g_runtime_global_context.registry.view<Transform3DComponent, Camera3DComponent>();
        g_runtime_global_context.registry.destroy(camera_view.begin(), camera_view.end());

        g_runtime_global_context.renderer = nullptr;
    }

    void RenderSystem::Update(float frame_time)
    {
        // ------------------- camera -------------------

        UBOData ubo_data;

        for (auto [entity, camera_component] : g_runtime_global_context.registry.view<const Camera3DComponent>().each())
        {
            if (camera_component.is_main_camera)
            {
                ubo_data.view       = camera_component.view;
                ubo_data.projection = camera_component.projection;

                break;
            }
        }

        // ------------------- render -------------------

        g_runtime_global_context.renderer->UpdateUniformBuffer(ubo_data);

        uint32_t image_index;
        g_runtime_global_context.renderer->StartRenderpass(image_index);

        auto& cmd_buffer = g_runtime_global_context.renderer
                               ->m_command_buffers[g_runtime_global_context.renderer->m_current_frame_index];

        for (auto [entity, transfrom_component, model_component] :
             g_runtime_global_context.registry.view<const Transform3DComponent, ModelComponent>().each())
        {
            // TODO: How to solve the different uniform buffer problem?
            // ubo_data.model = transfrom_component.m_global_transform;

            model_component.model.Draw(cmd_buffer);
        }

        g_runtime_global_context.renderer->EndRenderpass(image_index);
    }
} // namespace Meow
