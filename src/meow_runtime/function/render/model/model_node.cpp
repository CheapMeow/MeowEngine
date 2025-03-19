#include "model_node.h"

namespace Meow
{
    glm::mat4& ModelNode::GetGlobalMatrix()
    {
        global_matrix = local_matrix;

        if (parent)
        {
            global_matrix = parent->GetGlobalMatrix() * global_matrix;
        }

        return global_matrix;
    }

    void ModelNode::CalcBounds(BoundingBox& outBounds)
    {
        if (meshes.size() > 0)
        {
            const glm::mat4& matrix = GetGlobalMatrix();
            for (size_t i = 0; i < meshes.size(); ++i)
            {
                glm::vec3 mmin = matrix * glm::vec4(meshes[i]->bounding.min, 1.0);
                glm::vec3 mmax = matrix * glm::vec4(meshes[i]->bounding.max, 1.0);

                outBounds.Merge(mmin, mmax);
            }
        }

        for (size_t i = 0; i < children.size(); ++i)
        {
            children[i]->CalcBounds(outBounds);
        }
    }

    BoundingBox ModelNode::GetBounds()
    {
        glm::vec3 min(
            std::numeric_limits<float>().max(), std::numeric_limits<float>().max(), std::numeric_limits<float>().max());
        glm::vec3   max(-std::numeric_limits<float>().max(),
                      -std::numeric_limits<float>().max(),
                      -std::numeric_limits<float>().max());
        BoundingBox bounds(min, max);
        CalcBounds(bounds);
        bounds.UpdateCorners();
        return bounds;
    }
} // namespace Meow
