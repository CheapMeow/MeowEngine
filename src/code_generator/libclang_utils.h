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
        static std::string toStdString(CXString cxStr);

        static std::vector<std::string> split(const std::string& text, char delim);

        static void print_diagnostics(CXTranslationUnit TU);
    };

    std::ostream& operator<<(std::ostream& stream, const CXString& str);
} // namespace Meow
