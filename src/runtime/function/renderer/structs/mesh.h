#pragma once

#include "core/math/bounding_box.h"
#include "primitive.h"

namespace Meow
{
    struct Mesh
    {
        std::vector<Primitive> primitives;
        BoundingBox            bounding;

        int32_t vertex_count = 0;
        int32_t index_count  = 0;

        Mesh(std::vector<Primitive>&& _primitives, BoundingBox _bounding = BoundingBox())
            : primitives(std::move(_primitives))
            , bounding(_bounding)
        {
            for (Primitive& primitive : primitives)
            {
                vertex_count += primitive.vertex_count;
                index_count += primitive.index_count;
            }
        }

        void BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer) const
        {
            for (int i = 0; i < primitives.size(); ++i)
            {
                primitives[i].BindDrawCmd(cmd_buffer);
            }
        }
    };

} // namespace Meow