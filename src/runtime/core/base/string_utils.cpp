#include "string_utils.h"

namespace Meow
{
    std::string RemoveClassAndNamespace(const std::string& full_type_name)
    {
        // Split the string by spaces
        std::stringstream        ss(full_type_name);
        std::string              item;
        std::vector<std::string> tokens;

        while (ss >> item)
        {
            tokens.push_back(item);
        }

        // Get the last substring (after the last space)
        std::string& last_token = tokens.back();

        // Find the position of the last "::" in the last substring
        size_t last_colon_pos = last_token.rfind("::");

        // If "::" is found, return the part after it; otherwise, return the whole last substring
        return (last_colon_pos != std::string::npos) ? last_token.substr(last_colon_pos + 2) : last_token;
    }
} // namespace Meow