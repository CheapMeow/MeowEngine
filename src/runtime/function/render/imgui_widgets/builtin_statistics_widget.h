#pragma once

#include "function/render/structs/builtin_render_stat.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Meow
{
    class BuiltinStatisticsWidget
    {
    public:
        static void Draw(const std::unordered_map<std::string, BuiltinRenderStat>& stat);

    private:
        static void DrawFrameStat(const BuiltinRenderStat& stat);
        static void DrawVertexAttributesStat(const BuiltinRenderStat& stat);
        static void DrawBufferStat(const BuiltinRenderStat& stat);
        static void DrawImageStat(const BuiltinRenderStat& stat);
    };
} // namespace Meow