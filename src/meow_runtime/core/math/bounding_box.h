#pragma once

#include <glm/glm.hpp>

namespace Meow
{
    struct BoundingBox
    {
        glm::vec3 min;
        glm::vec3 max;

        glm::vec3 corners[8];

        BoundingBox()
            : min(0.0, 0.0, 0.0)
            , max(0.0, 0.0, 0.0)
        {}

        BoundingBox(const glm::vec3& _min, const glm::vec3& _max)
            : min(_min)
            , max(_max)
        {}

        void Merge(const BoundingBox& rhs)
        {
            min.x = glm::min(min.x, rhs.min.x);
            min.y = glm::min(min.y, rhs.min.y);
            min.z = glm::min(min.z, rhs.min.z);

            max.x = glm::max(max.x, rhs.max.x);
            max.y = glm::max(max.y, rhs.max.y);
            max.z = glm::max(max.z, rhs.max.z);
        }

        void Merge(const glm::vec3& _min, const glm::vec3& _max)
        {
            min.x = glm::min(min.x, _min.x);
            min.y = glm::min(min.y, _min.y);
            min.z = glm::min(min.z, _min.z);

            max.x = glm::max(max.x, _max.x);
            max.y = glm::max(max.y, _max.y);
            max.z = glm::max(max.z, _max.z);
        }

        void UpdateCorners()
        {
            corners[0] = glm::vec3(min.x, min.y, min.z);
            corners[1] = glm::vec3(max.x, min.y, min.z);
            corners[2] = glm::vec3(min.x, max.y, min.z);
            corners[3] = glm::vec3(max.x, max.y, min.z);

            corners[4] = glm::vec3(min.x, min.y, max.z);
            corners[5] = glm::vec3(max.x, min.y, max.z);
            corners[6] = glm::vec3(min.x, max.y, max.z);
            corners[7] = glm::vec3(max.x, max.y, max.z);
        }
    };
} // namespace Meow