#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace Meow
{
    struct KeywordResult
    {
        bool is_reflectable_found = false;
        bool is_enum_found        = false;
    };

    class KeywordFinder
    {
    public:
        static KeywordResult Find(const fs::path& filePath);
    };
} // namespace Meow