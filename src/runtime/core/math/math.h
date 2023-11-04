#pragma once

#include "core/base/macro.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
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

        static glm::quat QuaternionFromAngleAxis(const float angle, const glm::vec3 axis)
        {
            // ASSERT:  axis[] is unit length
            //
            // The quaternion representing the rotation is
            //   q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)
            float half_angle = 0.5 * angle;
            float sin_v      = glm::sin(half_angle);
            return {glm::cos(half_angle), sin_v * axis.x, sin_v * axis.y, sin_v * axis.z};
        }
    };
} // namespace Meow