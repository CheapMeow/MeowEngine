#pragma once

#include "function/ecs/system.h"

#include <filesystem>
#include <string>
#include <vector>

namespace Meow
{
    class FileSystem final : public System
    {
    public:
        // TODO: Support configure of engine root path
        FileSystem() { m_root_path = "E:/repositories/MeowEngine"; }

        void ReadBinaryFile(std::string const& filepath, uint8_t*& data_ptr, uint32_t& data_size);

    private:
        std::filesystem::path m_root_path;
    };
} // namespace Meow