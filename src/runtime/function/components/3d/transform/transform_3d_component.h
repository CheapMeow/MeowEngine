#pragma once

#include "function/components/component.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Meow
{
    struct Transform3DComponent : Component
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale    = glm::vec3(1.0f);

        /**
         * @brief Get the Transform from position, rotation(quaternion) and scale
         *
         * Construct Ordering:
         *
         * 1. Scale
         *
         * 2. Rotate
         *
         * 3. Translate
         *
         * @return glm::mat4
         */
        glm::mat4 GetTransform() const
        {
            glm::mat4 transform;

            glm::mat3 rotation_mat = glm::mat3_cast(rotation);

            // Set up final matrix with scale, rotation and translation
            transform[0][0] = scale.x * rotation_mat[0][0];
            transform[0][1] = scale.y * rotation_mat[0][1];
            transform[0][2] = scale.z * rotation_mat[0][2];
            transform[0][3] = position.x;
            transform[1][0] = scale.x * rotation_mat[1][0];
            transform[1][1] = scale.y * rotation_mat[1][1];
            transform[1][2] = scale.z * rotation_mat[1][2];
            transform[1][3] = position.y;
            transform[2][0] = scale.x * rotation_mat[2][0];
            transform[2][1] = scale.y * rotation_mat[2][1];
            transform[2][2] = scale.z * rotation_mat[2][2];
            transform[2][3] = position.z;

            // No projection term
            transform[3][0] = 0;
            transform[3][1] = 0;
            transform[3][2] = 0;
            transform[3][3] = 1;

            return transform;
        }
    };
} // namespace Meow