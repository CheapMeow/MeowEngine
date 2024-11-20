#pragma once

#include "meow_runtime/core/base/bitmask.hpp"
#include "meow_runtime/function/render/structs/shader.h"
#include "meow_runtime/function/render/render_resources/vertex_attribute.h"

namespace Meow
{
    struct BuiltinRenderStat
    {
        int draw_call = 0;

        std::vector<VertexAttributeMeta>            vertex_attribute_metas;
        std::unordered_map<std::string, BufferMeta> buffer_meta_map;
        std::unordered_map<std::string, ImageMeta>  image_meta_map;
    };
} // namespace Meow