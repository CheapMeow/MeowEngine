#include "code_generator.h"

#include "utils/code_gen_utils.h"

#include <iomanip>

namespace Meow
{
    void CodeGenerator::Begin(const std::string& src_path, const std::string& output_path)
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

        this->src_path    = fs::path(src_path);
        this->output_path = fs::path(output_path);
    }

    void CodeGenerator::Generate(const std::vector<std::string>&      include_relative_paths,
                                 const std::vector<ClassParseResult>& class_results,
                                 const std::vector<EnumParseResult>&  enum_results)
    {
        output_source_file << "#include \"register_all.h\"\n\n";
        output_source_file << "#include \"core/reflect/type_descriptor_builder.hpp\"\n";

        for (const auto& include_relative_path : include_relative_paths)
        {
            output_source_file << "#include \"" << include_relative_path << "\"\n";
        }

        output_source_file << std::endl;
        output_source_file << "namespace Meow" << std::endl;
        output_source_file << "{" << std::endl;
        output_source_file << '\t' << "void RegisterAll()" << std::endl;
        output_source_file << '\t' << "{" << std::endl;

        bool                     is_first           = true;
        static const std::string add_field_nonarray = ".AddFieldNonArray(\"";
        static const std::string add_field_array    = ".AddFieldArray(\"";

        for (const auto& class_result : class_results)
        {
            // seperate each part
            if (!is_first)
                output_source_file << std::endl;

            is_first = false;

            output_source_file << "\t\t" << "reflect::AddClass<" << class_result.class_name << ">("
                               << std::quoted(class_result.class_name) << ")";

            for (const auto& field_result : class_result.field_results)
            {
                if (field_result.is_array)
                {
                    output_source_file << "\n\t\t\t" << ".AddArray(" << std::quoted(field_result.field_name) << ", "
                                       << std::quoted(field_result.field_type_name) << ", "
                                       << std::quoted(field_result.inner_type_name) << ", &" << class_result.class_name
                                       << "::" << field_result.field_name << ")";
                }
                else
                {
                    output_source_file << "\n\t\t\t" << ".AddField(" << std::quoted(field_result.field_name) << ", "
                                       << std::quoted(field_result.field_type_name) << ", &" << class_result.class_name
                                       << "::" << field_result.field_name << ")";
                }
            }

            for (const auto& method_result : class_result.method_results)
            {
                output_source_file << "\n\t\t\t" << ".AddMethod(" << std::quoted(method_result.method_name) << ", &"
                                   << class_result.class_name << "::" << method_result.method_name << ")";
            }

            output_source_file << ";\n";
        }

        output_source_file << '\t' << "}" << std::endl;
        output_source_file << std::endl;

        for (const auto& enum_result : enum_results)
        {
            GenerateEnumReflHeaderFile(enum_result);

            std::stringstream gen_src_stream1;
            std::stringstream gen_src_stream2;

            gen_src_stream1 << "\t" << enum_result.enum_name << " to_enum(const std::string& str)" << std::endl;
            gen_src_stream1 << "\t{";

            gen_src_stream2 << "\tconst std::string to_string(" << enum_result.enum_name << " enum_val)" << std::endl;
            gen_src_stream2 << "\t{" << std::endl;
            gen_src_stream2 << "\t\tswitch (enum_val)" << std::endl;
            gen_src_stream2 << "\t\t{";

            for (const auto& enum_element_name : enum_result.enum_element_names)
            {
                gen_src_stream1 << "\n\t\tif (str == " << std::quoted(enum_element_name) << ")";
                gen_src_stream1 << "\n\t\t\treturn " << enum_result.enum_name << "::" << enum_element_name << ";";

                gen_src_stream2 << "\n\t\t\tcase " << enum_result.enum_name << "::" << enum_element_name << ":";
                gen_src_stream2 << "\n\t\t\t\treturn " << std::quoted(enum_element_name) << ";";
            }

            gen_src_stream1 << std::endl;
            gen_src_stream1 << std::endl;
            gen_src_stream1 << "\t\treturn " << enum_result.enum_name << "::None;" << std::endl;
            gen_src_stream1 << "\t}" << std::endl;
            gen_src_stream1 << std::endl;

            gen_src_stream2 << "\n\t\t\tdefault:" << std::endl;
            gen_src_stream2 << "\t\t\t\treturn \"Unknown\";" << std::endl;
            gen_src_stream2 << "\t\t}" << std::endl;
            gen_src_stream2 << "\t}" << std::endl;
            gen_src_stream2 << std::endl;

            output_source_file << gen_src_stream1.str() << gen_src_stream2.str();
        }

        output_source_file << "} // namespace Meow" << std::endl;
        output_source_file.close();
    }

    void CodeGenerator::End()
    {
        if (!is_recording)
        {
            std::cerr << "Parser already ends recording." << std::endl;
            return;
        }

        is_recording = false;
    }

    void CodeGenerator::GenerateEnumReflHeaderFile(const EnumParseResult& enum_result)
    {
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

        std::string replaced_header = CodeGenUtils::replace_all(gen_header_template, "EnumName", enum_result.enum_name);
        replaced_header =
            CodeGenUtils::replace_all(replaced_header, "UnderlyingType", enum_result.underlying_type_name);

        std::string   gen_header_file_name = CodeGenUtils::camel_case_to_under_score(enum_result.enum_name);
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