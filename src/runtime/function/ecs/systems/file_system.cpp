#include "file_system.h"

#include <cstring>
#include <fstream>

namespace Meow
{
    void FileSystem::ReadBinaryFile(std::string const& filepath, uint8_t*& data_ptr, uint32_t& data_size)
    {
        std::ifstream ifs(m_root_path / filepath, std::ios::binary | std::ios::ate);

        if (!ifs)
            throw std::runtime_error(filepath + ": " + std::strerror(errno));

        auto end = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        data_size = std::size_t(end - ifs.tellg());

        if (data_size == 0) // avoid undefined behavior
        {
            data_ptr  = nullptr;
            data_size = 0;
            return;
        }

        data_ptr = new uint8_t[data_size];

        if (!ifs.read((char*)data_ptr, data_size))
            throw std::runtime_error(filepath + ": " + std::strerror(errno));
    }
} // namespace Meow
