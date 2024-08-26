#pragma once

#include <imgui.h>
#include <string>

namespace Meow
{
    class SpinnerWidget
    {
    public:
        static bool Draw(std::string label, float radius, int thickness, const ImU32& color);
    };
} // namespace Meow