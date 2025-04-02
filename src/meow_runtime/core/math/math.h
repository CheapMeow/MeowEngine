#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Meow
{
    class Math
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

        static glm::quat QuaternionFromAngleAxis(const float angle, const glm::vec3 axis);

        static glm::mat4 perspective_vk(float fovy, float aspect, float zNear, float zFar);

        static bool
        DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
    };
} // namespace Meow