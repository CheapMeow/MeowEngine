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
        static void Draw(const std::unordered_map<std::string, BuiltinRenderStat>& stat, size_t& id);
    };
} // namespace Meow