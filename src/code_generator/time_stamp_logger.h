#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace Meow
{
    class TimeStampLogger
    {
        using timestamp_t = std::chrono::nanoseconds::rep;

    public:
        TimeStampLogger(){}

        bool LoadLog(fs::path log_path);

        bool IsModified(const std::vector<fs::path>& files);

        bool OutputLog(const std::vector<fs::path>& files, fs::path log_path) const;

    private:
        std::unordered_map<std::string, timestamp_t> m_time_stamp_map;
    };
} // namespace Meow