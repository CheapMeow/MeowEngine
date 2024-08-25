#include "libclang_utils.h"

#include <cctype> // for std::tolower

namespace Meow
{
    std::string LibclangUtils::to_string(CXString cxStr)
    {
        std::string result = clang_getCString(cxStr);
        clang_disposeString(cxStr);
        return result;
    }

    std::vector<std::string> LibclangUtils::split(const std::string& text, char delim)
    {
        std::string              line;
        std::vector<std::string> vec;
        std::stringstream        ss(text);
        while (std::getline(ss, line, delim))
        {
            vec.push_back(line);
        }
        return vec;
    }

    void LibclangUtils::print_diagnostics(CXTranslationUnit TU)
    {
        unsigned numDiagnostics = clang_getNumDiagnostics(TU);
        for (unsigned i = 0; i < numDiagnostics; ++i)
        {
            CXDiagnostic diag    = clang_getDiagnostic(TU, i);
            CXString     diagStr = clang_formatDiagnostic(diag, clang_defaultDiagnosticDisplayOptions());
            printf("Diagnostic %u: %s\n", i, clang_getCString(diagStr));
            clang_disposeString(diagStr);
            clang_disposeDiagnostic(diag);
        }
    }

    std::string LibclangUtils::replace_all(std::string subject, const std::string& search, const std::string& replace)
    {
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::string::npos)
        {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }

    std::string LibclangUtils::camel_case_to_under_score(const std::string& camel_case_str)
    {
        std::string under_score_str;

        for (char ch : camel_case_str)
        {
            if (std::isupper(ch))
            {
                if (!under_score_str.empty())
                {
                    under_score_str += '_';
                }
                ch = std::tolower(ch);
            }
            under_score_str += ch;
        }

        return under_score_str;
    }

    std::ostream& operator<<(std::ostream& stream, const CXString& str)
    {
        stream << clang_getCString(str);
        clang_disposeString(str);
        return stream;
    }
} // namespace Meow