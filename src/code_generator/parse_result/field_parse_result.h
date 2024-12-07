#pragma once

#include <string>

namespace Meow
{
    struct FieldParseResult
    {
        std::string field_type_name;
        bool        is_array;
        std::string inner_type_name;
        std::string field_name;
    };
} // namespace Meow