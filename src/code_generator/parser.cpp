#include "parser.h"

#include <algorithm>
#include <functional>

namespace Meow
{
    void print_diagnostics(CXTranslationUnit TU)
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

    void Parser::Begin(const std::string& src_root, const std::string& output_path)
    {
        if (is_recording)
        {
            std::cerr << "Parser is already recording." << std::endl;
            return;
        }

        output_file = std::ofstream(output_path + "/register_all.cpp");
        if (!output_file)
        {
            std::cerr << "Error opening or creating the file " << output_path + "/register_all.cpp" << std::endl;
            return;
        }

        is_recording = true;
        class_name_set.clear();

        src_root_path = fs::path(src_root);

        include_stream << "#include \"register_all.h\"\n\n";
        include_stream << "#include \"core/reflect/type_descriptor_builder.hpp\"\n";
    }

    void Parser::ParseFile(const fs::path& path, const std::vector<std::string>& include_paths)
    {
        // traverse AST to find class

        CXIndex index = clang_createIndex(0, 0);

        std::vector<const char*> all_args(1 + include_paths.size());
        all_args[0] = "-xc++";
        for (int i = 0; i < include_paths.size(); i++)
        {
            all_args[i + 1] = include_paths[i].c_str();
        }
        CXTranslationUnit unit = clang_parseTranslationUnit(
            index, path.string().c_str(), all_args.data(), all_args.size(), nullptr, 0, CXTranslationUnit_None);
        if (unit == nullptr)
        {
            std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
            exit(-1);
        }

        // print_diagnostics(unit);

        std::vector<CXCursor> class_cursors;

        CXCursor cursor = clang_getTranslationUnitCursor(unit);
        clang_visitChildren(
            cursor,
            [](CXCursor c, CXCursor parent, CXClientData client_data) {
                std::vector<CXCursor>* context_ptr = static_cast<std::vector<CXCursor>*>(client_data);

                if (clang_getCursorKind(c) == CXCursor_AnnotateAttr)
                {
                    std::vector<std::string> annotations = split(toStdString(clang_getCursorSpelling(c)), ';');
                    if (annotations.size() == 0)
                        return CXChildVisit_Recurse;

                    if (annotations[0] == "reflectable_class")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_ClassDecl)
                        {
                            context_ptr->push_back(parent);
                        }
                    }
                    else if (annotations[0] == "reflectable_struct")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_StructDecl)
                        {
                            context_ptr->push_back(parent);
                        }
                    }
                }

                return CXChildVisit_Recurse;
            },
            &class_cursors);

        bool has_class_registered = false;

        // traverse AST to find field and method
        for (auto& cursor : class_cursors)
        {
            bool success         = ParseClass(path, cursor);
            has_class_registered = success || has_class_registered;
        }

        // Generate code: insert include path
        if (has_class_registered)
        {
            InsertIncludePath(path);
        }

        clang_disposeTranslationUnit(unit);
        clang_disposeIndex(index);
    }

    void Parser::End()
    {
        if (!is_recording)
        {
            std::cerr << "Parser already ends recording." << std::endl;
            return;
        }

        output_file << include_stream.str();

        output_file << std::endl;
        output_file << "namespace Meow" << std::endl;
        output_file << "{" << std::endl;
        output_file << '\t' << "void RegisterAll()" << std::endl;
        output_file << '\t' << "{" << std::endl;

        output_file << register_stream.str();

        output_file << '\t' << "}" << std::endl;
        output_file << "} // namespace Meow" << std::endl;
        output_file.close();

        is_recording = false;
    }

    std::string Parser::toStdString(CXString cxStr)
    {
        std::string result = clang_getCString(cxStr);
        clang_disposeString(cxStr);
        return result;
    }

    std::vector<std::string> Parser::split(const std::string& text, char delim)
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

    void Parser::InsertIncludePath(const fs::path& path)
    {
        fs::path    file_path_relative = fs::relative(path, src_root_path);
        std::string include_path_rel   = file_path_relative.string();
        std::replace(include_path_rel.begin(), include_path_rel.end(), '\\', '/');

        include_stream << "#include \"" << include_path_rel << "\"\n";
    }

    bool Parser::ParseClass(const fs::path& path, CXCursor class_cursor)
    {
        std::string class_name = toStdString(clang_getCursorSpelling(class_cursor));

        std::cout << "[CodeGenerator] Parsing " << class_name << std::endl;

        // avoid repeating registering
        if (class_name_set.find(class_name) != class_name_set.end())
        {
            return false;
        }
        else
        {
            class_name_set.insert(class_name);
        }

        register_stream << "\t\t" << "reflect::AddClass<" << class_name << ">(\"" << class_name << "\")";

        struct ParseContext
        {
            std::stringstream* stream_ptr;
            std::string        class_name;
        };

        ParseContext parse_context = {&register_stream, class_name};

        clang_visitChildren(
            class_cursor,
            [](CXCursor c, CXCursor parent, CXClientData client_data) {
                if (clang_getCursorKind(c) == CXCursor_AnnotateAttr)
                {
                    ParseContext* parse_context_ptr = static_cast<ParseContext*>(client_data);

                    std::vector<std::string> annotations = split(toStdString(clang_getCursorSpelling(c)), ';');
                    if (annotations.size() == 0)
                        return CXChildVisit_Recurse;

                    if (annotations[0] == "reflectable_field")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_FieldDecl)
                        {
                            CXType      field_type      = clang_getCursorType(parent);
                            std::string field_type_name = toStdString(clang_getTypeSpelling(field_type));

                            std::string field_name = toStdString(clang_getCursorSpelling(parent));
                            *(parse_context_ptr->stream_ptr)
                                << "\n\t\t\t" << ".AddField(\"" << field_name << "\", \"" << field_type_name << "\", &"
                                << parse_context_ptr->class_name << "::" << field_name << ")";
                        }
                    }
                    else if (annotations[0] == "reflectable_method")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_CXXMethod)
                        {
                            std::string method_name = toStdString(clang_getCursorSpelling(parent));
                            *(parse_context_ptr->stream_ptr)
                                << "\n\t\t\t" << ".AddMethod(\"" << method_name << "\", &"
                                << parse_context_ptr->class_name << "::" << method_name << ")";
                        }
                    }
                }

                return CXChildVisit_Recurse;
            },
            &parse_context);

        register_stream << ";\n\n";

        return true;
    }

} // namespace Meow
