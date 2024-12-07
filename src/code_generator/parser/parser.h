#pragma once

#include "parse_result/class_parse_result.h"
#include "parse_result/enum_parse_result.h"

#include <clang-c/Index.h> // This is libclang.
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace Meow
{
    class Parser
    {
    public:
        void Begin(const std::string& src_root);

        void ParseFile(const fs::path& path, const std::vector<std::string>& include_paths);

        void End();

        const std::vector<std::string>& GetInlcudeRelativePaths() const { return include_relative_paths; }

        const std::vector<ClassParseResult>& GetClassResults() const { return class_results; }

        const std::vector<EnumParseResult>& GetEnumResults() const { return enum_results; }

    private:
        bool ParseClass(const fs::path& path, CXCursor class_cursor);

        bool ParseEnum(const fs::path& path, CXCursor enum_cursor);

        bool                            is_recording = false;
        fs::path                        src_path;
        std::unordered_set<std::string> class_name_set;
        std::unordered_set<std::string> enum_name_set;

        std::vector<std::string>      include_relative_paths;
        std::vector<ClassParseResult> class_results;
        std::vector<EnumParseResult>  enum_results;
    };
} // namespace Meow