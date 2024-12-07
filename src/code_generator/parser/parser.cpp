#include "parser.h"

#include "utils/code_gen_utils.h"

#include <functional>

namespace Meow
{
    void Parser::Begin(const std::string& src_path)
    {
        is_recording   = true;
        this->src_path = fs::path(src_path);
        class_name_set.clear();
    }

    void Parser::ParseFile(const fs::path& path, const std::vector<std::string>& include_paths)
    {
        // traverse AST to find class

        CXIndex index = clang_createIndex(0, 0);

        std::vector<const char*> all_args(3 + include_paths.size());
        all_args[0] = "-xc++";
        all_args[1] = "-std=c++20";
        all_args[2] = "-DGLM_ENABLE_EXPERIMENTAL";
        for (int i = 0; i < include_paths.size(); i++)
        {
            all_args[i + 3] = include_paths[i].c_str();
        }
        CXTranslationUnit unit = clang_parseTranslationUnit(
            index, path.string().c_str(), all_args.data(), all_args.size(), nullptr, 0, CXTranslationUnit_None);
        if (unit == nullptr)
        {
            std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
            exit(-1);
        }

        CodeGenUtils::print_diagnostics(unit);

        struct ParseContext
        {
            std::vector<CXCursor> class_cursors;
            std::vector<CXCursor> enum_cursors;
        };

        ParseContext parse_context;

        CXCursor cursor = clang_getTranslationUnitCursor(unit);
        clang_visitChildren(
            cursor,
            [](CXCursor c, CXCursor parent, CXClientData client_data) {
                ParseContext* context_ptr = static_cast<ParseContext*>(client_data);

                if (clang_getCursorKind(c) == CXCursor_AnnotateAttr)
                {
                    std::vector<std::string> annotations =
                        CodeGenUtils::split(CodeGenUtils::to_string(clang_getCursorSpelling(c)), ';');
                    if (annotations.size() == 0)
                        return CXChildVisit_Recurse;

                    if (annotations[0] == "reflectable_class")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_ClassDecl)
                        {
                            context_ptr->class_cursors.push_back(parent);
                        }
                    }
                    else if (annotations[0] == "reflectable_struct")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_StructDecl)
                        {
                            context_ptr->class_cursors.push_back(parent);
                        }
                    }
                    else if (annotations[0] == "reflectable_enum")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_EnumDecl)
                        {
                            context_ptr->enum_cursors.push_back(parent);
                        }
                    }
                }

                return CXChildVisit_Recurse;
            },
            &parse_context);

        bool has_class_registered = false;

        // traverse AST to find field and method
        for (auto& cursor : parse_context.class_cursors)
        {
            bool success         = ParseClass(path, cursor);
            has_class_registered = success || has_class_registered;
        }

        bool has_enum_registered = false;
        for (auto& cursor : parse_context.enum_cursors)
        {
            bool success        = ParseEnum(path, cursor);
            has_enum_registered = success || has_enum_registered;
        }

        if (has_class_registered || has_enum_registered)
        {
            include_relative_paths.push_back(CodeGenUtils::get_relative_path(path, src_path));
        }

        clang_disposeTranslationUnit(unit);
        clang_disposeIndex(index);
    }

    void Parser::End() { is_recording = false; }

    bool Parser::ParseClass(const fs::path& path, CXCursor class_cursor)
    {
        static const std::string vector_prefix = "std::vector<";

        std::string class_name = CodeGenUtils::to_string(clang_getCursorSpelling(class_cursor));

        std::cout << "[CodeGenerator] Parsing reflectable class " << class_name << std::endl;

        // avoid repeating registering
        if (class_name_set.find(class_name) != class_name_set.end())
        {
            return false;
        }
        else
        {
            class_name_set.insert(class_name);
        }

        ClassParseResult class_result;
        class_result.class_name = class_name;

        clang_visitChildren(
            class_cursor,
            [](CXCursor c, CXCursor parent, CXClientData client_data) {
                if (clang_getCursorKind(c) == CXCursor_AnnotateAttr)
                {
                    ClassParseResult* class_result = static_cast<ClassParseResult*>(client_data);

                    std::vector<std::string> annotations =
                        CodeGenUtils::split(CodeGenUtils::to_string(clang_getCursorSpelling(c)), ';');
                    if (annotations.size() == 0)
                        return CXChildVisit_Recurse;

                    if (annotations[0] == "reflectable_field")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_FieldDecl)
                        {
                            CXType      field_type      = clang_getCursorType(parent);
                            std::string field_type_name = CodeGenUtils::to_string(clang_getTypeSpelling(field_type));

                            bool        is_array        = field_type_name.find(vector_prefix) == 0;
                            std::string inner_type_name = CodeGenUtils::get_name_without_container(field_type_name);

                            std::string field_name = CodeGenUtils::to_string(clang_getCursorSpelling(parent));

                            class_result->field_results.emplace_back(
                                field_type_name, is_array, inner_type_name, field_name);
                        }
                    }
                    else if (annotations[0] == "reflectable_method")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_CXXMethod)
                        {
                            std::string method_name = CodeGenUtils::to_string(clang_getCursorSpelling(parent));
                            class_result->method_results.emplace_back(method_name);
                        }
                    }
                }

                return CXChildVisit_Recurse;
            },
            &class_result);

        class_results.push_back(class_result);

        return true;
    }

    bool Parser::ParseEnum(const fs::path& path, CXCursor enum_cursor)
    {
        std::string enum_name = CodeGenUtils::to_string(clang_getCursorSpelling(enum_cursor));
        // avoid repeating registering
        if (enum_name_set.find(enum_name) != enum_name_set.end())
        {
            return false;
        }

        CXType      underlying_type      = clang_getEnumDeclIntegerType(enum_cursor);
        std::string underlying_type_name = CodeGenUtils::to_string(clang_getTypeSpelling(underlying_type));

        EnumParseResult enum_result;
        enum_result.enum_name            = enum_name;
        enum_result.underlying_type_name = underlying_type_name;

        std::stringstream gen_src_stream1;
        std::stringstream gen_src_stream2;

        clang_visitChildren(
            enum_cursor,
            [](CXCursor c, CXCursor parent, CXClientData client_data) {
                EnumParseResult* enum_result = static_cast<EnumParseResult*>(client_data);

                if (clang_getCursorKind(c) == CXCursor_EnumConstantDecl)
                {
                    std::string enum_element_name = CodeGenUtils::to_string(clang_getCursorSpelling(c));
                    enum_result->enum_element_names.push_back(enum_element_name);
                }

                return CXChildVisit_Recurse;
            },
            &enum_result);

        if (enum_result.enum_element_names.size() == 0)
            return false;

        std::cout << "[CodeGenerator] Parsing reflectable enum " << enum_name << std::endl;

        // success find
        // only then can we insert to map
        enum_name_set.insert(enum_name);
        enum_results.push_back(enum_result);

        return true;
    }
} // namespace Meow
