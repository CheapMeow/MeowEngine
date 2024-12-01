#pragma once

#include "function/system.h"

#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

#ifndef ENGINE_ROOT_DIR
#    define ENGINE_ROOT_DIR ""
#endif

namespace Meow
{
    class FileSystem final : public System
    {
    public:
        // TODO: Support configure of engine root path
        FileSystem() { m_root_path = ENGINE_ROOT_DIR; }

        void Start() override;

        /**
         * @brief Get the absolute path
         *
         * @param path Relative path.
         * @return std::string Absolute path.
         */
        std::string GetAbsolutePath(const std::string& path)
        {
            std::filesystem::path full_path = m_root_path / path;
            full_path                       = full_path.lexically_normal();
            return full_path.string();
        }

        /**
         * @brief Is relative path existing.
         *
         * @param path Relative path.
         * @return true Path exists;
         * @return false Path doesn't exist.
         */
        bool Exists(const std::filesystem::path& path)
        {
            std::filesystem::path full_path = m_root_path / path;
            return std::filesystem::exists(full_path);
        }

        /**
         * @brief Read binary file by relative path.
         *
         * @param file_path Relative path.
         * @return std::tuple<uint8_t*, uint32_t> data_ptr, data_size
         */
        std::tuple<uint8_t*, uint32_t> ReadBinaryFile(std::string const& file_path);

        std::tuple<uint32_t, uint32_t> GetImageFileWidthHeight(std::string const& file_path);

        /**
         * @brief Read image file by relative path.
         *
         * @param file_path Relative path.
         * @param data_ptr Pointer to receive data loaded.
         * @return uint32_t Data size.
         */
        uint32_t ReadImageRGBA(std::string const& file_path, uint8_t* data_ptr);

        uint32_t ReadImageFloat(std::string const& file_path, uint8_t* data_ptr);

    private:
        std::filesystem::path m_root_path;
    };
} // namespace Meow