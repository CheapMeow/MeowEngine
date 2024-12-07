#pragma once

#include <string>
#include <vector>

namespace Meow
{
    struct EnumParseResult
    {
        std::string              enum_name;
        std::string              underlying_type_name;
        std::vector<std::string> enum_element_names;
    };

} // namespace Meow