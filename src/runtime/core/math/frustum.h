#pragma once

#include "bounding_box.h"
#include "plane.h"

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
        void updatePlanes(glm::mat4& viewMat, const glm::vec3& cameraPos, float fov, float AR, float near, float far);
        bool checkIfInside(BoundingBox* bounds);

    private:
        Plane pl[6];
    };
} // namespace Meow