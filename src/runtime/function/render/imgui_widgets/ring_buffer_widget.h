#pragma once

#include <imgui.h>
#include <string>

#include "function/render/structs/material.h"

namespace Meow
{
    class RingBufferWidget
    {
    public:
        RingBufferWidget();

        void Draw(const std::unordered_map<std::string, RingUniformBufferStat>& ringbuf_stat_map);

    private:
        static constexpr int    k_gredint_count     = 20;
        static constexpr double k_gredint_partition = 1.0 / 20.0;

        std::vector<ImU32> m_col_base_table;
        std::vector<ImU32> m_col_hovered_table;

        static constexpr ImU32 col_outline = 0xFFFFFFFF;
    };
} // namespace Meow