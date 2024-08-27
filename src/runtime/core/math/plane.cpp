#include "plane.h"

namespace Meow
{
    float Plane::distance(const glm::vec3& points) { return glm::dot(normal, points) + D; }

    void Plane::setNormalAndPoint(const glm::vec3& n, const glm::vec3& p0)
    {
        normal = n;
        D      = -glm::dot(n, p0);
    }
} // namespace Meow