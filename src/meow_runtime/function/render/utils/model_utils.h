#pragma once

#include "function/render/render_resources/model.hpp"

#include <tuple>
#include <vector>

namespace Meow
{
    std::tuple<std::vector<float>, std::vector<uint32_t>>
    GenerateSphereVerticesAndIndices(uint32_t                        x_segments,
                                     uint32_t                        y_segments,
                                     std::vector<VertexAttributeBit> attributes);
} // namespace Meow
