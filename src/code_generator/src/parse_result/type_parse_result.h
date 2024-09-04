#pragma once

#include "field_parse_result.h"
#include "method_parse_result.h"

#include <string>
#include <vector>

namespace Meow
{
    struct TypeParseResult
    {
        std::string                    class_name;
        std::vector<FieldParseResult>  field_results;
        std::vector<MethodParseResult> method_results;
    };
} // namespace Meow