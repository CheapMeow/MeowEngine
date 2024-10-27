#pragma once

#include <glm/glm.hpp>

namespace Meow
{
    struct Plane
    {
        glm::vec3 normal;
        float     D;

        float distance(const glm::vec3& points);
        void  setNormalAndPoint(const glm::vec3& normal, const glm::vec3& point);
    };
} // namespace Meow