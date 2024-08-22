#include "parser.h"
#include "time_stamp_logger.h"

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
    std::string              log_path    = "";

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
        else if (arg.substr(0, 2) == "-L" && arg.size() > 2)
        {
            if (log_path.size() > 0)
            {
                std::cerr << "[CodeGenerator] More than one -L<log_path>!" << std::endl;
                return 1;
            }
            log_path = arg.substr(2);
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

    if (!fs::exists(log_path))
    {
        std::cout << "[CodeGenerator] log_path does not exist!" << std::endl;
    }
    else if (!fs::is_regular_file(log_path))
    {
        std::cout << "[CodeGenerator] log_path is not a file!" << std::endl;
    }

    std::cout << "[CodeGenerator] src_path is" << std::endl << src_path << std::endl;
    std::cout << "[CodeGenerator] output_path is" << std::endl << output_path << std::endl;
    std::cout << "[CodeGenerator] log_path is" << std::endl << log_path << std::endl;
    std::cout << "[CodeGenerator] include_path is" << std::endl;
    for (int i = 0; i < include_paths.size(); i++)
    {
        std::cout << include_paths[i] << std::endl;
    }

    Parser          parser;
    TimeStampLogger logger;

    std::unordered_set<std::string> suffixes = {".h", ".hpp"};
    std::vector<fs::path>           files;
    for (const auto& entry : fs::recursive_directory_iterator(src_path))
    {
        if (entry.is_regular_file() && suffixes.find(entry.path().extension().string()) != suffixes.end())
        {
            if (parser.ContainsReflectableKeywords(entry.path()))
                files.push_back(entry.path());
        }
    }

    logger.LoadLog(log_path);
    if (logger.IsModified(files))
    {
        std::cout << "[CodeGenerator] Reflectable files are modified, begin parsing." << std::endl;

        parser.Begin(src_path, output_path);
        for (int i = 0; i < files.size(); ++i)
        {
            parser.ParseFile(files[i], include_paths);
        }
        parser.End();
        logger.OutputLog(files, log_path);
    }
    else
    {
        std::cout << "[CodeGenerator] Reflectable files are not modified, skip parsing." << std::endl;
    }
}