#pragma once

#include <clang-c/Index.h> // This is libclang.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Meow
{
    class CodeGenUtils
    {
    public:
        static bool find_reflectable_keyword(const fs::path& filePath);

        static std::string to_string(CXString cxStr);

        static std::vector<std::string> split(const std::string& text, char delim);

        static void print_diagnostics(CXTranslationUnit TU);

        static std::string replace_all(std::string subject, const std::string& search, const std::string& replace);

        static std::string camel_case_to_under_score(const std::string& camel_case_str);

        static std::string get_name_without_container(std::string name);
    };

    std::ostream& operator<<(std::ostream& stream, const CXString& str);
} // namespace Meow
