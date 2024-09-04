#include "parser.h"

#include "code_gen_utils.h"

#include <algorithm>
#include <functional>

namespace Meow
{
    void Parser::Begin(const std::string& src_path, const std::string& output_path)
    {
        if (is_recording)
        {
            std::cerr << "Parser is already recording." << std::endl;
            return;
        }

        output_source_file = std::ofstream(output_path + "/register_all.cpp");
        if (!output_source_file)
        {
            std::cerr << "Error opening or creating the file " << output_path + "/register_all.cpp" << std::endl;
            return;
        }

        is_recording = true;
        class_name_set.clear();

        this->src_path    = fs::path(src_path);
        this->output_path = fs::path(output_path);

        include_stream << "#include \"register_all.h\"\n\n";
        include_stream << "#include \"core/reflect/type_descriptor_builder.hpp\"\n";
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

        output_source_file << include_stream.str();

        output_source_file << std::endl;
        output_source_file << "namespace Meow" << std::endl;
        output_source_file << "{" << std::endl;
        output_source_file << '\t' << "void RegisterAll()" << std::endl;
        output_source_file << '\t' << "{" << std::endl;

        output_source_file << register_stream.str();

        output_source_file << '\t' << "}" << std::endl;
        output_source_file << std::endl;

        output_source_file << enum_stream.str();

        output_source_file << "} // namespace Meow" << std::endl;
        output_source_file.close();

        is_recording = false;
    }

    void Parser::InsertIncludePath(const fs::path& path)
    {
        fs::path    file_path_relative = fs::relative(path, src_path);
        std::string include_path_rel   = file_path_relative.string();
        std::replace(include_path_rel.begin(), include_path_rel.end(), '\\', '/');

        include_stream << "#include \"" << include_path_rel << "\"\n";
    }

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

        // seperate each part
        if (register_stream.str().length() != 0)
            register_stream << std::endl;

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
                            *(parse_context_ptr->stream_ptr)
                                << "\n\t\t\t" << ".AddField(\"" << field_name << "\", \"" << field_type_name << "\", &"
                                << parse_context_ptr->class_name << "::" << field_name << ")";
                        }
                    }
                    else if (annotations[0] == "reflectable_method")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_CXXMethod)
                        {
                            std::string method_name = CodeGenUtils::to_string(clang_getCursorSpelling(parent));
                            *(parse_context_ptr->stream_ptr)
                                << "\n\t\t\t" << ".AddMethod(\"" << method_name << "\", &"
                                << parse_context_ptr->class_name << "::" << method_name << ")";
                        }
                    }
                }

                return CXChildVisit_Recurse;
            },
            &parse_context);

        register_stream << ";\n";

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

        struct ParseContext
        {
            std::stringstream* stream_ptr2;
            std::stringstream* stream_ptr3;
            std::string        enum_name;
            bool               has_found = false;
        };

        std::stringstream gen_src_stream2;
        std::stringstream gen_src_stream3;

        gen_src_stream2 << "\t" << enum_name << " to_enum(const std::string& str)" << std::endl;
        gen_src_stream2 << "\t{";

        gen_src_stream3 << "\tconst std::string to_string(" << enum_name << " enum_val)" << std::endl;
        gen_src_stream3 << "\t{" << std::endl;
        gen_src_stream3 << "\t\tswitch (enum_val)" << std::endl;
        gen_src_stream3 << "\t\t{";

        ParseContext parse_context = {&gen_src_stream2, &gen_src_stream3, enum_name};

        clang_visitChildren(
            enum_cursor,
            [](CXCursor c, CXCursor parent, CXClientData client_data) {
                ParseContext* parse_context_ptr = static_cast<ParseContext*>(client_data);

                if (clang_getCursorKind(c) == CXCursor_EnumConstantDecl)
                {
                    parse_context_ptr->has_found = true;

                    std::string enum_element_name = CodeGenUtils::to_string(clang_getCursorSpelling(c));

                    *(parse_context_ptr->stream_ptr2) << "\n\t\tif (str == \"" << enum_element_name << "\")";
                    *(parse_context_ptr->stream_ptr2)
                        << "\n\t\t\treturn " << parse_context_ptr->enum_name << "::" << enum_element_name << ";";

                    *(parse_context_ptr->stream_ptr3)
                        << "\n\t\t\tcase " << parse_context_ptr->enum_name << "::" << enum_element_name << ":";
                    *(parse_context_ptr->stream_ptr3) << "\n\t\t\t\treturn \"" << enum_element_name << "\";";
                }

                return CXChildVisit_Recurse;
            },
            &parse_context);

        // declaration in .gen.h is behind of implementation
        // so the declaration with no element is scanned first
        // we should jump it
        if (parse_context.has_found == false)
            return false;

        std::cout << "[CodeGenerator] Parsing reflectable enum " << enum_name << std::endl;

        // success find
        // only then can we insert to map
        enum_name_set.insert(enum_name);

        GenerateEnumReflHeaderFile(path, enum_cursor);

        gen_src_stream2 << std::endl;
        gen_src_stream2 << std::endl;
        gen_src_stream2 << "\t\treturn " << enum_name << "::None;" << std::endl;
        gen_src_stream2 << "\t}" << std::endl;
        gen_src_stream2 << std::endl;

        gen_src_stream3 << "\n\t\t\tdefault:" << std::endl;
        gen_src_stream3 << "\t\t\t\treturn \"Unknown\";" << std::endl;
        gen_src_stream3 << "\t\t}" << std::endl;
        gen_src_stream3 << "\t}" << std::endl;
        gen_src_stream3 << std::endl;

        enum_stream << gen_src_stream2.str() << gen_src_stream3.str();

        return true;
    }

    void Parser::GenerateEnumReflHeaderFile(const fs::path& path, CXCursor enum_cursor)
    {
        std::string enum_name            = CodeGenUtils::to_string(clang_getCursorSpelling(enum_cursor));
        CXType      underlying_type      = clang_getEnumDeclIntegerType(enum_cursor);
        std::string underlying_type_name = CodeGenUtils::to_string(clang_getTypeSpelling(underlying_type));

        std::string gen_header_template = R"(#pragma once  

#include "core/reflect/macros.h"

#include <cstdint>
#include <string>  
  
namespace Meow  
{  
    enum class EnumName : UnderlyingType;  
  
    EnumName to_enum(const std::string& str);  
  
    const std::string to_string(EnumName enum_val);  
} // namespace Meow  
)";

        std::string replaced_header = CodeGenUtils::replace_all(gen_header_template, "EnumName", enum_name);
        replaced_header = CodeGenUtils::replace_all(replaced_header, "UnderlyingType", underlying_type_name);

        std::string   gen_header_file_name = CodeGenUtils::camel_case_to_under_score(enum_name);
        std::ofstream output_header_file(output_path.string() + "/" + gen_header_file_name + ".gen.h");
        if (output_header_file.is_open())
        {
            output_header_file << replaced_header;
            output_header_file.close();
            std::cout << "[CodeGenerator] Generated: " << output_path.string() + "/" + gen_header_file_name + ".gen.h"
                      << std::endl;
        }
        else
        {
            std::cerr << "[CodeGenerator] Fail to write: "
                      << output_path.string() + "/" + gen_header_file_name + ".gen.h" << std::endl;
        }
    }
} // namespace Meow
