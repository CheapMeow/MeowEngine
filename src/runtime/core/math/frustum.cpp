#include "frustum.h"

// Otherwise #defines like M_PI are undeclared under Visual Studio
#define _USE_MATH_DEFINES

#include <math.h>

// C99 removes M_PI from math.h
#ifndef M_PI
#    define M_PI 3.14159265358979323846264338327
#endif

namespace Meow
{

    // Calculates frustum planes in world space
    void
    Frustum::updatePlanes(glm::mat4& viewMat, const glm::vec3& cameraPos, float fov, float AR, float near, float far)
    {
        float tanHalfFOV = tan((fov / 2) * (M_PI / 180));
        float nearH      = near * tanHalfFOV; // Half of the frustum near plane height
        float nearW      = nearH * AR;

        glm::vec3 X(viewMat[0]); // Side Unit Vector
        glm::vec3 Y(viewMat[1]); // Up Unit Vector
        glm::vec3 Z(viewMat[2]); // Forward vector

        // Gets worlds space position of the center points of the near and far planes
        // The forward vector Z points towards the viewer so you need to negate it and scale it
        // by the distance (near or far) to the plane to get the center positions
        glm::vec3 nearCenter = cameraPos + Z * near;
        glm::vec3 farCenter  = cameraPos + Z * far;

        glm::vec3 point;
        glm::vec3 normal;

        // We build the planes using a normal and a point (in this case the center)
        // Z is negative here because we want the normal vectors we choose to point towards
        // the inside of the view frustum that way we can cehck in or out with a simple
        // Dot product
        pl[NEARP].setNormalAndPoint(Z, nearCenter);
        // Far plane
        pl[FARP].setNormalAndPoint(-Z, farCenter);

        // Again, want to get the plane from a normal and point
        // You scale the up vector by the near plane height and added to the nearcenter to
        // optain a point on the edge of both near and top plane.
        // Subtracting the cameraposition from this point generates a vector that goes along the
        // surface of the plane, if you take the cross product with the direction vector equal
        // to the shared edge of the planes you get the normal
        point  = nearCenter + Y * nearH;
        normal = glm::normalize(point - cameraPos);
        normal = glm::cross(normal, X);
        pl[TOP].setNormalAndPoint(normal, point);

        // Bottom plane
        point  = nearCenter - Y * nearH;
        normal = glm::normalize(point - cameraPos);
        normal = glm::cross(normal, X);
        pl[BOTTOM].setNormalAndPoint(normal, point);

        // Left plane
        point  = nearCenter - X * nearW;
        normal = glm::normalize(point - cameraPos);
        normal = glm::cross(normal, Y);
        pl[LEFT].setNormalAndPoint(normal, point);

        // Right plane
        point  = nearCenter + X * nearW;
        normal = glm::normalize(point - cameraPos);
        normal = glm::cross(normal, Y);
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