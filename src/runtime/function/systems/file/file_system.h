#pragma once

#include "function/systems/system.h"

#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

namespace Meow
{
    class FileSystem final : public System
    {
    public:
        // TODO: Support configure of engine root path
        FileSystem() { m_root_path = "E:/repositories/MeowEngine"; }

        std::tuple<uint8_t*, uint32_t> ReadBinaryFile(std::string const& filepath);

        uint32_t ReadImageFileToPtr(std::string const& filepath, uint8_t* data_ptr);

    private:
        std::filesystem::path m_root_path;
    };
} // namespace Meow