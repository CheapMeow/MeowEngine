#include "vulkan_math_utils.hpp"

namespace vk
{
    namespace Meow
    {
        glm::mat4x4 CreateModelViewProjectionClipMatrix(vk::Extent2D const& extent)
        {
            float fov = glm::radians(45.0f);
            if (extent.width > extent.height)
            {
                fov *= static_cast<float>(extent.height) / static_cast<float>(extent.width);
            }

            glm::mat4x4 model = glm::mat4x4(1.0f);
            glm::mat4x4 view =
                glm::lookAt(glm::vec3(-5.0f, 3.0f, -10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
            glm::mat4x4 projection = glm::perspective(fov, 1.0f, 0.1f, 100.0f);
            // clang-format off
            glm::mat4x4 clip = glm::mat4x4( 1.0f,  0.0f, 0.0f, 0.0f,
                                            0.0f, -1.0f, 0.0f, 0.0f,
                                            0.0f,  0.0f, 0.5f, 0.0f,
                                            0.0f,  0.0f, 0.5f, 1.0f );  // vulkan clip space has inverted y and half z !
            // clang-format on 
            return clip * projection * view * model;
        }
    }
}