#include "frustum.h"

#include <format>
#include <iostream>
#include <math.h>

namespace Meow
{
    // Calculates frustum planes in world space
    void Frustum::updatePlanes(const glm::vec3 cameraPos,
                               const glm::quat rotation,
                               float           fovy,
                               float           AR,
                               float           near,
                               float           far)
    {
        float tanHalfFOVy = tan(fovy / 2.0f);
        float near_height = near * tanHalfFOVy; // Half of the frustum near plane height
        float near_width  = near_height * AR;

        glm::vec3 right   = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 up      = glm::vec3(0.0f, 1.0f, 0.0f);

        // Gets worlds space position of the center points of the near and far planes
        // The forward vector Z points towards the viewer so you need to negate it and scale it
        // by the distance (near or far) to the plane to get the center positions
        glm::vec3 nearCenter = cameraPos + forward * near;
        glm::vec3 farCenter  = cameraPos + forward * far;

        glm::vec3 point;
        glm::vec3 normal;

        // We build the planes using a normal and a point (in this case the center)
        // Z is negative here because we want the normal vectors we choose to point towards
        // the inside of the view frustum that way we can cehck in or out with a simple
        // Dot product
        pl[NEARP].setNormalAndPoint(forward, nearCenter);

        // Far plane
        pl[FARP].setNormalAndPoint(-forward, farCenter);

        // Again, want to get the plane from a normal and point
        // You scale the up vector by the near plane height and added to the nearcenter to
        // optain a point on the edge of both near and top plane.
        // Subtracting the cameraposition from this point generates a vector that goes along the
        // surface of the plane, if you take the cross product with the direction vector equal
        // to the shared edge of the planes you get the normal
        point  = nearCenter + up * near_height;
        normal = glm::normalize(point - cameraPos);
        normal = glm::cross(right, normal);
        pl[TOP].setNormalAndPoint(normal, point);

        // Bottom plane
        point  = nearCenter - up * near_height;
        normal = glm::normalize(point - cameraPos);
        normal = glm::cross(normal, right);
        pl[BOTTOM].setNormalAndPoint(normal, point);

        // Left plane
        point  = nearCenter - right * near_width;
        normal = glm::normalize(point - cameraPos);
        normal = glm::cross(up, normal);
        pl[LEFT].setNormalAndPoint(normal, point);

        // Right plane
        point  = nearCenter + right * near_width;
        normal = glm::normalize(point - cameraPos);
        normal = glm::cross(normal, up);
        pl[RIGHT].setNormalAndPoint(normal, point);
    }

    // False is fully outside, true if inside or intersects
    // based on iquilez method
    bool Frustum::checkIfInside(BoundingBox* box)
    {
        // Check box outside or inside of frustum
        for (int i = 0; i < 6; ++i)
        {
            int out = 0;
            out += ((pl[i].distance(glm::vec3(box->min.x, box->min.y, box->min.z)) < 0.0) ? 1 : 0);
            out += ((pl[i].distance(glm::vec3(box->max.x, box->min.y, box->min.z)) < 0.0) ? 1 : 0);
            out += ((pl[i].distance(glm::vec3(box->min.x, box->max.y, box->min.z)) < 0.0) ? 1 : 0);
            out += ((pl[i].distance(glm::vec3(box->max.x, box->max.y, box->min.z)) < 0.0) ? 1 : 0);
            out += ((pl[i].distance(glm::vec3(box->min.x, box->min.y, box->max.z)) < 0.0) ? 1 : 0);
            out += ((pl[i].distance(glm::vec3(box->max.x, box->min.y, box->max.z)) < 0.0) ? 1 : 0);
            out += ((pl[i].distance(glm::vec3(box->min.x, box->max.y, box->max.z)) < 0.0) ? 1 : 0);
            out += ((pl[i].distance(glm::vec3(box->max.x, box->max.y, box->max.z)) < 0.0) ? 1 : 0);

            if (out == 8)
                return false;
        }
        return true;
    }
} // namespace Meow