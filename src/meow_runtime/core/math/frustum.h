#pragma once

#include "bounding_box.h"
#include "plane.h"

#include <glm/gtc/quaternion.hpp>

namespace Meow
{
    class Frustum
    {
    private:
        enum
        {
            TOP = 0,
            BOTTOM,
            LEFT,
            RIGHT,
            NEARP,
            FARP
        };

    public:
        void
        updatePlanes(const glm::vec3 cameraPos, const glm::quat rotation, float fovy, float AR, float near, float far);
        bool checkIfInside(BoundingBox* bounds);

    private:
        Plane pl[6];
    };
} // namespace Meow