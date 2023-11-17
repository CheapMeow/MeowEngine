#include "file_system.h"

#include "core/log/log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>
#include <fstream>

namespace Meow
{
    std::tuple<uint8_t*, uint32_t> FileSystem::ReadBinaryFile(std::string const& filepath)
    {
        std::ifstream ifs(m_root_path / filepath, std::ios::binary | std::ios::ate);

        if (!ifs)
            throw std::runtime_error(filepath + ": " + std::strerror(errno));

        auto end = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        uint32_t data_size = std::size_t(end - ifs.tellg());

        if (data_size == 0) // avoid undefined behavior
        {
            return {nullptr, 0};
        }

        uint8_t* data_ptr = new uint8_t[data_size];

        if (!ifs.read((char*)data_ptr, data_size))
            throw std::runtime_error(filepath + ": " + std::strerror(errno));

        return {data_ptr, data_size};
    }

    uint32_t FileSystem::ReadImageFileToPtr(std::string const& filepath, uint8_t* data_ptr)
    {
        int texWidth, texHeight, texChannels;

        auto absolute_filepath = m_root_path / filepath;
        absolute_filepath      = absolute_filepath.lexically_normal();

        stbi_uc* pixels =
            stbi_load(absolute_filepath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        uint32_t data_size = texWidth * texHeight * 4;

        if (!pixels)
        {
            RUNTIME_WARN("Failed to load texture file: {}", filepath);
            return 0;
        }

        memcpy(data_ptr, pixels, data_size);

        return data_size;
    }
} // namespace Meow
