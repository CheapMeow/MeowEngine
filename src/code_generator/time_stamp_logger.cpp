#include "time_stamp_logger.h"

namespace Meow
{
    bool TimeStampLogger::LoadLog(fs::path log_path)
    {
        if (!fs::exists(log_path))
        {
            return false;
        }

        std::ifstream log_file(log_path);
        if (!log_file.is_open())
        {
            std::cerr << "Failed to open the log file: " << log_path << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(log_file, line))
        {
            std::istringstream line_stream(line);
            std::string        file_path_str;
            timestamp_t        last_write_time;

            if (std::getline(line_stream, file_path_str, ',') && line_stream >> last_write_time)
            {
                m_time_stamp_map[file_path_str] = last_write_time;
            }
        }

        return true;
    }

    bool TimeStampLogger::IsModified(const std::vector<fs::path>& files)
    {
        std::unordered_map<std::string, bool> visited;

        for (const auto& file : files)
        {
            if (!fs::exists(file))
            {
                continue;
            }

            auto abs_path        = fs::absolute(file).string();
            auto last_write_time = fs::last_write_time(file).time_since_epoch().count();

            if (m_time_stamp_map.find(abs_path) == m_time_stamp_map.end())
            {
                return false;
            }

            if (m_time_stamp_map[abs_path] < last_write_time)
            {
                return false;
            }

            visited[abs_path] = true;
        }

        for (const auto& [path, _] : m_time_stamp_map)
        {
            if (visited.find(path) == visited.end())
            {
                return false;
            }
        }

        return true;
    }

    bool TimeStampLogger::OutputLog(const std::vector<fs::path>& files, fs::path log_path) const
    {
        std::ofstream log_file(log_path);
        if (!log_file.is_open())
        {
            std::cerr << "Failed to open the log file: " << log_path << std::endl;
            return false;
        }

        for (const auto& file : files)
        {
            if (!fs::exists(file))
            {
                continue;
            }

            auto abs_path        = fs::absolute(file).string();
            auto last_write_time = fs::last_write_time(file).time_since_epoch().count();

            log_file << abs_path << "," << last_write_time << "\n";
        }

        return true;
    }
} // namespace Meow