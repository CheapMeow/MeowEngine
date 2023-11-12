#pragma once

#include "core/math/bounding_box.h"
#include "primitive.h"
#include "renderable.h"

namespace Meow
{
    struct Mesh : Renderable
    {
        std::vector<std::shared_ptr<Primitive>> primitives;
        std::string                             material_name = "Default Material";
        BoundingBox                             bounding;

        int32_t vertex_count = 0;
        int32_t index_count  = 0;

        void BindDrawCmd(vk::raii::CommandBuffer const& cmd_buffer) const override
        {
            for (size_t i = 0; i < primitives.size(); ++i)
            {
                primitives[i]->BindDrawCmd(cmd_buffer);
            }
        }
    };

} // namespace Meow