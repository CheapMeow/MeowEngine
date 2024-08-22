#include "parser.h"

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
    std::string              src_root    = "";
    std::string              output_root = "";

    int include_path_count = 0;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg.substr(0, 2) == "-S" && arg.size() > 2)
        {
            if (src_root.size() > 0)
            {
                std::cerr << "More than one -S<src_root>!" << std::endl;
                return 1;
            }
            src_root = arg.substr(2);
        }
        else if (arg.substr(0, 2) == "-O" && arg.size() > 2)
        {
            if (output_root.size() > 0)
            {
                std::cerr << "More than one -O<output_root>!" << std::endl;
                return 1;
            }
            output_root = arg.substr(2);
        }
        else if (arg.substr(0, 2) == "-I" && arg.size() > 2)
        {
            include_paths.push_back(arg);
        }
    }

    if (!std::filesystem::is_directory(src_root))
    {
        std::cerr << "src_root is invalid!" << std::endl;
        exit(-1);
    }
    if (!std::filesystem::is_directory(output_root))
    {
        std::cerr << "output_root is invalid!" << std::endl;
        exit(-1);
    }

    std::cout << "[CodeGenerator] src_root is" << std::endl << src_root << std::endl;
    std::cout << "[CodeGenerator] output_root is" << std::endl << output_root << std::endl;
    std::cout << "[CodeGenerator] include_path is" << std::endl;
    for (int i = 0; i < include_paths.size(); i++)
    {
        std::cout << include_paths[i] << std::endl;
    }

    std::unordered_set<std::string> suffixes = {".h", ".hpp"};
    std::vector<fs::path>           files;
    for (const auto& entry : fs::recursive_directory_iterator(src_root))
    {
        if (entry.is_regular_file() && suffixes.find(entry.path().extension().string()) != suffixes.end())
        {
            files.push_back(entry.path());
        }
    }

    Parser parser;
    parser.Begin(src_root, output_root);
    for (int i = 0; i < files.size(); ++i)
    {
        parser.ParseFile(files[i], include_paths);
    }
    parser.End();
}