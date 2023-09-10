#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

namespace vk
{
    namespace Meow
    {
        glm::mat4x4 CreateModelViewProjectionClipMatrix(vk::Extent2D const& extent);
    }
}