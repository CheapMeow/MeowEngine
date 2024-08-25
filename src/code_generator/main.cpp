#include "keyword_finder.h"
#include "type_descriptor_parser.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;
using namespace Meow;

int main(int argc, char* argv[])
{
    std::vector<std::string> include_paths;
    std::string              src_path    = "";
    std::string              output_path = "";

    int include_path_count = 0;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg.substr(0, 2) == "-S" && arg.size() > 2)
        {
            if (src_path.size() > 0)
            {
                std::cerr << "[CodeGenerator] More than one -S<src_path>!" << std::endl;
                return 1;
            }
            src_path = arg.substr(2);
        }
        else if (arg.substr(0, 2) == "-O" && arg.size() > 2)
        {
            if (output_path.size() > 0)
            {
                std::cerr << "[CodeGenerator] More than one -O<output_path>!" << std::endl;
                return 1;
            }
            output_path = arg.substr(2);
        }
        else if (arg.substr(0, 2) == "-I" && arg.size() > 2)
        {
            include_paths.push_back(arg);
        }
    }

    if (!fs::exists(src_path))
    {
        std::cerr << "[CodeGenerator] src_path does not exist!" << std::endl;
        exit(-1);
    }
    else if (!fs::is_directory(src_path))
    {
        std::cerr << "[CodeGenerator] src_path is not a directory!" << std::endl;
        exit(-1);
    }

    if (!fs::exists(output_path))
    {
        std::cerr << "[CodeGenerator] output_path does not exist!" << std::endl;
        exit(-1);
    }
    else if (!fs::is_directory(output_path))
    {
        std::cerr << "[CodeGenerator] output_path is not a directory!" << std::endl;
        exit(-1);
    }

    std::cout << "[CodeGenerator] src_path is" << std::endl << src_path << std::endl;
    std::cout << "[CodeGenerator] output_path is" << std::endl << output_path << std::endl;
    std::cout << "[CodeGenerator] include_path is" << std::endl;
    for (int i = 0; i < include_paths.size(); i++)
    {
        std::cout << include_paths[i] << std::endl;
    }

    TypeDescriptorParser type_descriptor_parser;

    std::vector<fs::path> reflectable_files;
    std::vector<fs::path> enum_files;

    std::unordered_set<std::string> suffixes = {".h", ".hpp"};
    for (const auto& entry : fs::recursive_directory_iterator(src_path))
    {
        if (entry.is_regular_file() && suffixes.find(entry.path().extension().string()) != suffixes.end())
        {
            auto result = KeywordFinder::Find(entry.path());
            if (result.is_reflectable_found)
                reflectable_files.push_back(entry.path());
            if (result.is_enum_found)
                enum_files.push_back(entry.path());
        }
    }

    type_descriptor_parser.Begin(src_path, output_path);
    for (int i = 0; i < reflectable_files.size(); ++i)
    {
        std::cout << "[CodeGenerator] Finding reflectable class in " << reflectable_files[i].string() << std::endl;
        type_descriptor_parser.ParseFile(reflectable_files[i], include_paths);
    }
    type_descriptor_parser.End();

    return 0;
}
