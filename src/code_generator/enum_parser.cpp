#include "enum_parser.h"

#include "libclang_utils.h"

#include <algorithm>
#include <functional>

namespace Meow
{
    void EnumParser::Begin(const std::string& src_path, const std::string& output_path)
    {
        if (is_recording)
        {
            std::cerr << "EnumParser is already recording." << std::endl;
            return;
        }

        is_recording = true;
        enum_name_set.clear();

        this->src_path    = fs::path(src_path);
        this->output_path = fs::path(output_path);
    }

    void EnumParser::ParseFile(const fs::path& path, const std::vector<std::string>& include_paths)
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

        // LibclangUtils::print_diagnostics(unit);

        std::vector<CXCursor> enum_cursors;

        CXCursor cursor = clang_getTranslationUnitCursor(unit);
        clang_visitChildren(
            cursor,
            [](CXCursor c, CXCursor parent, CXClientData client_data) {
                std::vector<CXCursor>* context_ptr = static_cast<std::vector<CXCursor>*>(client_data);

                if (clang_getCursorKind(c) == CXCursor_AnnotateAttr)
                {
                    std::vector<std::string> annotations =
                        LibclangUtils::split(LibclangUtils::to_string(clang_getCursorSpelling(c)), ';');
                    if (annotations.size() == 0)
                        return CXChildVisit_Recurse;

                    if (annotations[0] == "reflectable_enum")
                    {
                        if (clang_getCursorKind(parent) == CXCursor_EnumDecl)
                        {
                            context_ptr->push_back(parent);
                        }
                    }
                }

                return CXChildVisit_Recurse;
            },
            &enum_cursors);

        // traverse AST to find field and method
        for (auto& cursor : enum_cursors)
        {
            ParseEnum(path, cursor);
        }

        clang_disposeTranslationUnit(unit);
        clang_disposeIndex(index);
    }

    void EnumParser::End()
    {
        if (!is_recording)
        {
            std::cerr << "EnumParser already ends recording." << std::endl;
            return;
        }

        is_recording = false;
    }

    bool EnumParser::ParseEnum(const fs::path& path, CXCursor enum_cursor)
    {
        std::string enum_name = LibclangUtils::to_string(clang_getCursorSpelling(enum_cursor));

        std::cout << "[CodeGenerator] Parsing " << enum_name << std::endl;

        // avoid repeating registering
        if (enum_name_set.find(enum_name) != enum_name_set.end())
        {
            return false;
        }
        else
        {
            enum_name_set.insert(enum_name);
        }

        std::string gen_header_template = R"(#pragma once  
  
#include <string>  
  
namespace Meow  
{  
    enum class Foo;  
  
    Foo to_enum(const std::string& str);  
  
    const std::string to_string(Foo enum_val);  
} // namespace Meow  
)";

        std::string replaced_header = LibclangUtils::replace_all(gen_header_template, "Foo", enum_name);

        std::string   gen_header_file_name = LibclangUtils::camel_case_to_under_score(enum_name);
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

        struct ParseContext
        {
            std::stringstream* stream_ptr2;
            std::stringstream* stream_ptr3;
            std::string        enum_name;
        };

        std::stringstream gen_src_stream1;
        gen_src_stream1 << "#include \"" << gen_header_file_name + ".gen.h\"" << std::endl;
        gen_src_stream1 << std::endl;
        gen_src_stream1 << "namespace Meow" << std::endl;
        gen_src_stream1 << "{" << std::endl;

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
                if (clang_getCursorKind(c) == CXCursor_AnnotateAttr)
                {
                    ParseContext* parse_context_ptr = static_cast<ParseContext*>(client_data);

                    if (clang_getCursorKind(parent) == CXCursor_EnumConstantDecl)
                    {
                        std::string enum_element_name = LibclangUtils::to_string(clang_getCursorSpelling(c));

                        *(parse_context_ptr->stream_ptr2) << "\n\t\tif (str == \"" << enum_element_name << "\")";
                        *(parse_context_ptr->stream_ptr2)
                            << "\n\t\t\treturn " << parse_context_ptr->enum_name << "::" << enum_element_name << ";";

                        *(parse_context_ptr->stream_ptr3)
                            << "\n\t\t\tcase " << parse_context_ptr->enum_name << "::" << enum_element_name << ":";
                        *(parse_context_ptr->stream_ptr3) << "\n\t\t\t\treturn \"" << enum_element_name << "\";";
                    }
                }

                return CXChildVisit_Recurse;
            },
            &parse_context);

        gen_src_stream2 << "\n\t\t}" << std::endl;
        gen_src_stream2 << "\t}" << std::endl;
        gen_src_stream2 << std::endl;

        gen_src_stream3 << "\n\t\t\tdefault:" << std::endl;
        gen_src_stream3 << "\t\t\t\treturn \"Unknown\"" << std::endl;
        gen_src_stream3 << "\t\t" << std::endl;
        gen_src_stream3 << "\t" << std::endl;
        gen_src_stream3 << "} // namespace Meow" << std::endl;

        std::ofstream output_source_file(output_path.string() + "/" + gen_header_file_name + ".gen.cpp");
        if (output_source_file.is_open())
        {
            output_source_file << gen_src_stream1.str() << gen_src_stream2.str() << gen_src_stream3.str();
            output_source_file.close();
            std::cout << "[CodeGenerator] Generated: " << output_path.string() + "/" + gen_header_file_name + ".gen.cpp"
                      << std::endl;
        }
        else
        {
            std::cerr << "[CodeGenerator] Fail to write: "
                      << output_path.string() + "/" + gen_header_file_name + ".gen.cpp" << std::endl;
        }

        return true;
    }

} // namespace Meow
