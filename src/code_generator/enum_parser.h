#include <clang-c/Index.h> // This is libclang.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace Meow
{
    class EnumParser
    {
    public:
        void Begin(const std::string& src_root, const std::string& output_path);

        void ParseFile(const fs::path& path, const std::vector<std::string>& include_paths);

        void End();

    private:
        bool ParseEnum(const fs::path& path, CXCursor enum_cursor);

        bool                            is_recording = false;
        std::unordered_set<std::string> enum_name_set;

        fs::path          src_path;
        fs::path          output_path;
        std::stringstream register_stream;
    };
} // namespace Meow