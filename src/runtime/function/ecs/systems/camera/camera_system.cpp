#include "camera_system.h"

#include "function/ecs/components/3d/camera/camera_3d_component.h"
#include "function/ecs/components/3d/transform/transform_3d_component.h"
#include "function/global/runtime_global_context.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    void CameraSystem::Update(float frame_time)
    {
        for (auto [entity, transfrom_component, camera_component] :
             g_runtime_global_context.registry.view<const Transform3DComponent, Camera3DComponent>().each())
        {
            glm::ivec2 window_size = g_runtime_global_context.window->GetSize();

            camera_component.view       = glm::inverse(transfrom_component.global_transform);
            camera_component.projection = glm::perspectiveFovLH_ZO(camera_component.field_of_view,
                                                                   (float)window_size[0],
                                                                   (float)window_size[1],
                                                                   camera_component.near_plane,
                                                                   camera_component.far_plane);
        }
    }
} // namespace Meow
