#include "code_gen_utils.h"

#include <algorithm>
#include <cctype> // for std::tolower

namespace Meow
{
    bool CodeGenUtils::find_reflectable_keyword(const fs::path& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            std::cerr << "Failed to open the file: " << filePath << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("reflectable_class") != std::string::npos ||
                line.find("reflectable_struct") != std::string::npos ||
                line.find("reflectable_field") != std::string::npos ||
                line.find("reflectable_method") != std::string::npos ||
                line.find("reflectable_enum") != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    std::string CodeGenUtils::to_string(CXString cxStr)
    {
        std::string result = clang_getCString(cxStr);
        clang_disposeString(cxStr);
        return result;
    }

    std::string CodeGenUtils::get_relative_path(const fs::path& path, const fs::path& src_path)
    {
        fs::path    file_path_relative = fs::relative(path, src_path);
        std::string path_rel           = file_path_relative.string();
        std::replace(path_rel.begin(), path_rel.end(), '\\', '/');
        return path_rel;
    }

    std::vector<std::string> CodeGenUtils::split(const std::string& text, char delim)
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

    void CodeGenUtils::print_diagnostics(CXTranslationUnit TU)
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

    std::string CodeGenUtils::replace_all(std::string subject, const std::string& search, const std::string& replace)
    {
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::string::npos)
        {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }

    std::string CodeGenUtils::camel_case_to_under_score(const std::string& camel_case_str)
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

    std::string CodeGenUtils::get_name_without_container(std::string name)
    {
        size_t left  = name.find_first_of('<') + 1;
        size_t right = name.find_last_of('>');
        if (left > 0 && right < name.size() && left < right)
        {
            return name.substr(left, right - left);
        }
        else
        {
            return "";
        }
    }

    std::ostream& operator<<(std::ostream& stream, const CXString& str)
    {
        stream << clang_getCString(str);
        clang_disposeString(str);
        return stream;
    }
} // namespace Meow