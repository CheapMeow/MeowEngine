#pragma once

#include "core/base/macro.h"

#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

namespace Meow
{
    class LIBRARY_API Math
    {
    public:
        /**
         * Used to floor the value if less than the min.
         * @tparam T The values type.
         * @param min The minimum value.
         * @param value The value.
         * @return Returns a value with deadband applied.
         */
        template<typename T = float>
        static T Deadband(const T& min, const T& value)
        {
            return std::fabs(value) >= std::fabs(min) ? value : 0.0f;
        }

        static glm::mat4x4 CreateModelViewProjectionClipMatrix(vk::Extent2D const& extent)
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
    };
} // namespace Meow