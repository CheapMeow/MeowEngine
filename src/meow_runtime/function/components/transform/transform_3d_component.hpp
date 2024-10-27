#pragma once

#include "function/object/game_object.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Meow
{
    class [[reflectable_class()]] Transform3DComponent : public Component
    {
    public:
        [[reflectable_field()]]
        glm::vec3 position = glm::vec3(0.0f);

        [[reflectable_field()]]
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        [[reflectable_field()]]
        glm::vec3 scale = glm::vec3(1.0f);

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
            // caution: glm is col-major
            // Set up final matrix with scale, rotation and translation
            glm::mat4 transform          = glm::mat4(1.0f);
            glm::mat3 rotation_mat       = glm::mat3_cast(rotation);
            glm::mat3 scale_mat          = glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, scale.z));
            glm::mat3 rotation_scale_mat = scale_mat * rotation_mat;

            transform[0][0] = rotation_scale_mat[0][0];
            transform[0][1] = rotation_scale_mat[0][1];
            transform[0][2] = rotation_scale_mat[0][2];
            transform[1][0] = rotation_scale_mat[1][0];
            transform[1][1] = rotation_scale_mat[1][1];
            transform[1][2] = rotation_scale_mat[1][2];
            transform[2][0] = rotation_scale_mat[2][0];
            transform[2][1] = rotation_scale_mat[2][1];
            transform[2][2] = rotation_scale_mat[2][2];

            transform[3][0] = position.x;
            transform[3][1] = position.y;
            transform[3][2] = position.z;

            return transform;
        }
    };
} // namespace Meow