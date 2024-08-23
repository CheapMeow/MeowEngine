#pragma once

#include "core/time/scope_time_data.h"

#include <imgui.h>
#include <vector>

namespace Meow
{
    struct FlameGraphSnapshot
    {
        std::vector<ScopeTimeData> scope_times;
        int                        max_depth;
        std::chrono::microseconds  global_start;
        ImVec2                     graph_size;
    };

    class FlameGraphDrawer
    {
    public:
        void Draw(const std::vector<ScopeTimeData>& scope_times,
                  int                               max_depth,
                  std::chrono::microseconds         global_start,
                  ImVec2                            graph_size = ImVec2(0, 0));

    private:
        void Draw_impl(const std::vector<ScopeTimeData>& scope_times,
                       int                               max_depth,
                       std::chrono::microseconds         global_start,
                       ImVec2                            graph_size);

        bool               m_is_shapshot_enabled = false;
        FlameGraphSnapshot m_curr_shapshot;
    };
} // namespace Meow