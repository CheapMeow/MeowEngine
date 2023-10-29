#include "render_system.h"

#include "function/ecs/components/3d/camera/camera_3d_component.h"
#include "function/ecs/components/3d/model/model_component.h"
#include "function/ecs/components/3d/transform/transform_3d_component.h"
#include "function/global/runtime_global_context.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    RenderSystem::RenderSystem()
    {
        g_runtime_global_context.renderer = std::make_shared<VulkanRenderer>();

        // TODO: creating entity should be obstract
        const auto camera_entity = g_runtime_global_context.registry.create();
        g_runtime_global_context.registry.emplace<Transform3DComponent>(camera_entity);
        g_runtime_global_context.registry.emplace<Camera3DComponent>(camera_entity);

        const auto model_entity = g_runtime_global_context.registry.create();
        g_runtime_global_context.registry.emplace<Transform3DComponent>(model_entity);
        g_runtime_global_context.registry.emplace<ModelComponent>(
            model_entity,
            "builtin/models/monkey_head.obj",
            g_runtime_global_context.renderer->m_gpu,
            g_runtime_global_context.renderer->m_logical_device,
            std::vector<VertexAttribute> {VertexAttribute::VA_Position, VertexAttribute::VA_Normal});
    }

    RenderSystem::~RenderSystem()
    {
        g_runtime_global_context.renderer = nullptr;

        // TODO: deleting entity should be obstract
        auto model_view = g_runtime_global_context.registry.view<Transform3DComponent, ModelComponent>();
        g_runtime_global_context.registry.destroy(model_view.begin(), model_view.end());

        auto camera_view = g_runtime_global_context.registry.view<Transform3DComponent, Camera3DComponent>();
        g_runtime_global_context.registry.destroy(camera_view.begin(), camera_view.end());
    }

    void RenderSystem::Update(float frame_time)
    {
        // ------------------- camera -------------------

        UBOData ubo_data;

        for (auto [entity, transfrom_component, camera_component] :
             g_runtime_global_context.registry.view<const Transform3DComponent, Camera3DComponent>().each())
        {
            glm::ivec2 window_size = g_runtime_global_context.window->GetSize();

            camera_component.view       = glm::inverse(transfrom_component.m_global_transform);
            camera_component.projection = glm::perspectiveFovRH_ZO(camera_component.field_of_view,
                                                                   (float)window_size[0],
                                                                   (float)window_size[1],
                                                                   camera_component.near_plane,
                                                                   camera_component.far_plane);

            ubo_data.view       = camera_component.view;
            ubo_data.projection = camera_component.projection;
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
