#pragma once

#include "parse_result/class_parse_result.h"
#include "parse_result/enum_parse_result.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace Meow
{
    class CodeGenerator
    {
    public:
        void Begin(const std::string& src_root, const std::string& output_path);

        void Generate(const std::vector<std::string>&      include_relative_paths,
                      const std::vector<ClassParseResult>& class_results,
                      const std::vector<EnumParseResult>&  enum_results);

        void End();

    private:
        void GenerateEnumReflHeaderFile(const EnumParseResult& enum_result);

        bool              is_recording = false;
        fs::path          src_path;
        fs::path          output_path;
        std::ofstream     output_source_file;
        std::stringstream register_stream;
        std::stringstream enum_stream;
    };
} // namespace Meow
