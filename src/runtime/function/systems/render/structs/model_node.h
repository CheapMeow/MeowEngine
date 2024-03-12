#pragma once

#include "core/math/bounding_box.h"
#include "model_mesh.h"

#include <limits>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Meow
{
    struct ModelNode
    {
        std::string name;

        std::vector<ModelMesh*> meshes;

        ModelNode*              parent;
        std::vector<ModelNode*> children;

        glm::mat4 local_matrix;
        glm::mat4 global_matrix;

        ModelNode()
            : name("None")
            , parent(nullptr)
        {}

        const glm::mat4& GetLocalMatrix() { return local_matrix; }

        glm::mat4& GetGlobalMatrix();

        void CalcBounds(BoundingBox& outBounds);

        BoundingBox GetBounds();

        ~ModelNode()
        {
            for (size_t i = 0; i < meshes.size(); ++i)
            {
                delete meshes[i];
            }
            meshes.clear();

            for (size_t i = 0; i < children.size(); ++i)
            {
                delete children[i];
            }
            children.clear();
        }
    };
} // namespace Meow
