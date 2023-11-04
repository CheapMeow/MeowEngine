#pragma once

#include "core/math/bounding_box.h"
#include "primitive.h"

namespace Meow
{
    struct Mesh
    {
        std::vector<std::shared_ptr<Primitive>> primitives;
        BoundingBox                             bounding;

        Mesh(std::vector<std::shared_ptr<Primitive>> primitives, BoundingBox bounding)
        {
            this->primitives = primitives;
            this->bounding   = bounding;

            for (auto primitive : primitives)
            {
                vertex_count += primitive->vertex_count;
                index_count += primitive->index_count;
            }
        }

        int32_t vertex_count = 0;
        int32_t index_count  = 0;

        void BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer) const
        {
            for (int i = 0; i < primitives.size(); ++i)
            {
                primitives[i]->BindDrawCmd(cmd_buffer);
            }
        }
    };

} // namespace Meow