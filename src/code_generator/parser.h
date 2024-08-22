#include <clang-c/Index.h> // This is libclang.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace Meow
{
    class Parser
    {
    public:
        bool ContainsReflectableKeywords(const fs::path& filePath);

        void Begin(const std::string& src_root, const std::string& output_path);

        void ParseFile(const fs::path& path, const std::vector<std::string>& include_paths);

        void End();

    private:
        static std::string toStdString(CXString cxStr);

        static std::vector<std::string> split(const std::string& text, char delim);

        void InsertIncludePath(const fs::path& path);

        bool ParseClass(const fs::path& path, CXCursor class_cursor);

        bool                            is_recording = false;
        std::unordered_set<std::string> class_name_set;

        fs::path          src_path;
        std::ofstream     output_file;
        std::stringstream include_stream;
        std::stringstream register_stream;
    };
} // namespace Meow