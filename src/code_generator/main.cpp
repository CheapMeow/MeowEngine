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
    std::string include_path = "";
    std::string src_root     = "";
    std::string output_root  = "";

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
            if (include_path.size() > 0)
                include_path += " ";
            include_path += arg;
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

    std::cout << "src_root is " << src_root << std::endl;
    std::cout << "output_root is " << output_root << std::endl;
    std::cout << "include_path is " << include_path << std::endl;

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
        parser.ParseFile(files[i], include_path);
    }
    parser.End();
}