#include "generator/code_generator.h"
#include "parser/parser.h"
#include "utils/code_gen_utils.h"

#include <filesystem>
#include <iomanip>
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

    bool has_error = false;

    if (include_paths.size() == 0)
    {
        std::cerr << "[CodeGenerator] include_paths is empty!" << std::endl;
    }

    if (!fs::exists(src_path))
    {
        std::cerr << "[CodeGenerator] src_path " << std::quoted(src_path) << " does not exist!" << std::endl;
        has_error = true;
    }
    else if (!fs::is_directory(src_path))
    {
        std::cerr << "[CodeGenerator] src_path " << std::quoted(src_path) << " is not a directory!" << std::endl;
        has_error = true;
    }

    if (!fs::exists(output_path))
    {
        std::cerr << "[CodeGenerator] output_path " << std::quoted(output_path) << " does not exist!" << std::endl;
        has_error = true;
    }
    else if (!fs::is_directory(output_path))
    {
        std::cerr << "[CodeGenerator] output_path " << std::quoted(output_path) << " is not a directory!" << std::endl;
        has_error = true;
    }

    if (has_error)
        exit(-1);

    std::cout << "[CodeGenerator] src_path is" << std::endl << src_path << std::endl;
    std::cout << "[CodeGenerator] output_path is" << std::endl << output_path << std::endl;
    std::cout << "[CodeGenerator] include_path size = " << include_paths.size() << std::endl;
    for (int i = 0; i < include_paths.size(); i++)
    {
        std::cout << include_paths[i] << std::endl;
    }

    std::vector<fs::path> files;

    std::unordered_set<std::string> suffixes = {".h", ".hpp"};
    for (const auto& entry : fs::recursive_directory_iterator(src_path))
    {
        if (entry.is_regular_file() && suffixes.find(entry.path().extension().string()) != suffixes.end())
        {
            if (CodeGenUtils::find_reflectable_keyword(entry.path()))
                files.push_back(entry.path());
        }
    }

    Parser parser;
    parser.Begin(src_path);
    for (int i = 0; i < files.size(); ++i)
    {
        std::cout << "[CodeGenerator] Traversing " << files[i].string() << std::endl;
        parser.ParseFile(files[i], include_paths);
    }
    parser.End();

    CodeGenerator generator;
    generator.Begin(src_path, output_path);
    generator.Generate(parser.GetInlcudeRelativePaths(), parser.GetClassResults(), parser.GetEnumResults());
    generator.End();

    return 0;
}
