#pragma once

#include "function/render/structs/material_stat.h"

#include <cstdint>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace Meow
{
    class BuiltinStatisticsWidget
    {
    public:
        BuiltinStatisticsWidget();

        void Draw(const std::unordered_map<std::string, MaterialStat>& stat);

    private:
        static void DrawFrameStat(const MaterialStat& stat);
        static void DrawVertexAttributesStat(const MaterialStat& stat);
        static void DrawBufferStat(const MaterialStat& stat);
        static void DrawImageStat(const MaterialStat& stat);

        static constexpr int    k_gredint_count     = 20;
        static constexpr double k_gredint_partition = 1.0 / 20.0;

        std::vector<ImU32> m_col_base_table;
        std::vector<ImU32> m_col_hovered_table;

        static constexpr ImU32 col_outline = 0xFFFFFFFF;
    };
} // namespace Meow