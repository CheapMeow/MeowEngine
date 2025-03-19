#pragma once

#include "function/render/model/model.hpp"

#include <tuple>
#include <vector>

namespace Meow
{
    std::vector<float> GenerateCubeVertices();

    std::tuple<std::vector<float>, std::vector<uint32_t>>
    GenerateSphereVerticesAndIndices(uint32_t                        sector_count,
                                     uint32_t                        stack_count,
                                     float                           radius,
                                     std::vector<VertexAttributeBit> attributes);
} // namespace Meow
