#include "keyword_finder.h"

#include <fstream>
#include <iostream>

namespace Meow
{
    KeywordResult KeywordFinder::Find(const fs::path& filePath)
    {
        KeywordResult result;

        std::ifstream file(filePath);
        if (!file.is_open())
        {
            std::cerr << "Failed to open the file: " << filePath << std::endl;
            return result;
        }

        std::string line;
        while (std::getline(file, line))
        {
            if (!result.is_reflectable_found)
            {
                if (line.find("reflectable_class") != std::string::npos ||
                    line.find("reflectable_struct") != std::string::npos ||
                    line.find("reflectable_field") != std::string::npos ||
                    line.find("reflectable_method") != std::string::npos)
                {
                    result.is_reflectable_found = true;
                }
            }

            if (!result.is_enum_found)
            {
                if (line.find("reflectable_enum") != std::string::npos)
                {
                    result.is_enum_found = true;
                }
            }

            if (result.is_reflectable_found && result.is_enum_found)
            {
                return result;
            }
        }

        return result;
    }
} // namespace Meow