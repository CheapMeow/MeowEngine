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
    if (argc != 3)
    {
        std::cout << "argc = " << argc << std::endl;
        std::cout << "argv = " << std::endl;
        for (int i = 0; i < argc; ++i)
        {
            std::cout << argv[i] << std::endl;
        }
        std::cerr << "Input argument should be <src_root> <output_root>." << std::endl;
        exit(-1);
    }

    std::string src_root    = argv[1];
    std::string output_root = argv[2];

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
        parser.ParseFile(files[i]);
    }
    parser.End();
}