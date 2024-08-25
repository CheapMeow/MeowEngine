#pragma once

#include <clang-c/Index.h> // This is libclang.
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Meow
{
    class LibclangUtils
    {
    public:
        static std::string to_string(CXString cxStr);

        static std::vector<std::string> split(const std::string& text, char delim);

        static void print_diagnostics(CXTranslationUnit TU);

        static std::string replace_all(std::string subject, const std::string& search, const std::string& replace);

        static std::string camel_case_to_under_score(const std::string& camel_case_str);
    };

    std::ostream& operator<<(std::ostream& stream, const CXString& str);
} // namespace Meow
