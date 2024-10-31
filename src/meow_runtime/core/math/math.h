#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan.hpp>

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

        static glm::mat4 perspective_vk(float fovy, float aspect, float zNear, float zFar)
        {
            assert(abs(aspect - std::numeric_limits<float>::epsilon()) > static_cast<float>(0));

            float const tanHalfFovy = tan(fovy / 2.0f);

            glm::mat4 Result(0.0f);
            Result[0][0] = -1.0f / (aspect * tanHalfFovy);
            Result[1][1] = 1.0f / (tanHalfFovy);
            Result[2][2] = zFar / (zNear - zFar);
            Result[2][3] = -1.0f;
            Result[3][2] = -(zFar * zNear) / (zFar - zNear);
            return Result;
        }
    };
} // namespace Meow