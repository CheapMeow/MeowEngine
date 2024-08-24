#pragma once

#include "core/math/bounding_box.h"
#include "model_primitive.h"

#include <string>
#include <vector>

namespace Meow
{
    struct ModelNode;

    struct MaterialInfo
    {
        std::string diffuse;
        std::string normalmap;
        std::string specular;
    };

    struct ModelMesh
    {
        typedef std::vector<ModelPrimitive*> Primitives;

        Primitives  primitives;
        BoundingBox bounding;
        ModelNode*  link_node;

        std::vector<size_t> bones;
        bool                isSkin = false;

        MaterialInfo material;

        size_t vertex_count;
        size_t triangle_count;

        ModelMesh()
            : link_node(nullptr)
            , vertex_count(0)
            , triangle_count(0)
        {}

        void BindOnly(const vk::raii::CommandBuffer& cmd_buffer);

        void DrawOnly(const vk::raii::CommandBuffer& cmd_buffer);

        void BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer);

        ~ModelMesh()
        {
            for (int i = 0; i < primitives.size(); ++i)
            {
                delete primitives[i];
            }
            primitives.clear();
            link_node = nullptr;
        }
    };

} // namespace Meow
