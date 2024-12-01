#pragma once

#include "function/render/render_resources/vertex_attribute.h"
#include "function/render/structs/shader.h"

namespace Meow
{
    struct MaterialStat
    {
        uint32_t draw_call = 0;

        std::vector<VertexAttributeMeta>            vertex_attribute_metas;
        std::unordered_map<std::string, BufferMeta> buffer_meta_map;
        std::unordered_map<std::string, ImageMeta>  image_meta_map;
    };
} // namespace Meow