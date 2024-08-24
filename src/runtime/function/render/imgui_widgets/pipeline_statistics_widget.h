#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Meow
{
    class PipelineStatisticsWidget
    {
    public:
        static void Draw(const std::unordered_map<std::string, std::vector<uint32_t>>& stat);

    private:
        inline static std::string m_stat_name[11] = {"Vertex count in input assembly",
                                                     "Primitives count in input assembly",
                                                     "Vertex shader invocations",
                                                     "Geometry shader invocations",
                                                     "Primitives count in geometry shader",
                                                     "Clipping invocations",
                                                     "Clipping primtives",
                                                     "Fragment shader invocations",
                                                     "Tessellation control shader patches",
                                                     "Tessellation evaluation shader invocations",
                                                     "Compute shader invocations"};
    };
} // namespace Meow