#pragma once

#include <string>

namespace Meow
{
    struct MethodParseResult
    {
        std::string method_name;
        bool        has_js_binding = false;
    };
} // namespace Meow
