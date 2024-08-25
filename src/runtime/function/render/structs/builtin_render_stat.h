#pragma once

#include "core/base/bitmask.hpp"
#include "vertex_attribute.h"

namespace Meow
{
    struct BuiltinRenderStat
    {
        int draw_call = 0;

        BitMask<VertexAttributeBit> per_vertex_attributes;
        BitMask<VertexAttributeBit> instance_attributes;
    };
} // namespace Meow