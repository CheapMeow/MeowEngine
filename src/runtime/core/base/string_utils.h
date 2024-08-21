#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace Meow
{
    std::string RemoveClassAndNamespace(const std::string& full_type_name);
} // namespace Meow