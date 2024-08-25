#include "libclang_utils.h"

namespace Meow
{
    std::string LibclangUtils::toStdString(CXString cxStr)
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

    std::ostream& operator<<(std::ostream& stream, const CXString& str)
    {
        stream << clang_getCString(str);
        clang_disposeString(str);
        return stream;
    }
} // namespace Meow