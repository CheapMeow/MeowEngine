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
        inline static std::string m_attribute_names[17] = {"Position",
                                                     "UV0",
                                                     "UV1",
                                                     "Normal",
                                                     "Tangent",
                                                     "Color",
                                                     "SkinWeight",
                                                     "SkinIndex",
                                                     "SkinPack",
                                                     "InstanceFloat1",
                                                     "InstanceFloat2",
                                                     "InstanceFloat3",
                                                     "InstanceFloat4",
                                                     "Custom0",
                                                     "Custom1",
                                                     "Custom2",
                                                     "Custom3"};
    };
} // namespace Meow