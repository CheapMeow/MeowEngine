#pragma once

#include <assimp/scene.h>

#include <filesystem>
#include <string>
#include <vector>

namespace Meow
{
    struct PerMaterialTexturePaths
    {
        std::vector<std::string> diffuse_texture_paths;
        std::vector<std::string> normal_texture_paths;
        std::vector<std::string> specular_texture_paths;

        PerMaterialTexturePaths(const std::string& file_path, aiMaterial* material)
        {
            diffuse_texture_paths  = LoadMaterialTextures(file_path, material, aiTextureType::aiTextureType_DIFFUSE);
            normal_texture_paths   = LoadMaterialTextures(file_path, material, aiTextureType::aiTextureType_NORMALS);
            specular_texture_paths = LoadMaterialTextures(file_path, material, aiTextureType::aiTextureType_SPECULAR);
        }

    private:
        std::vector<std::string>
        LoadMaterialTextures(const std::string& file_path, const aiMaterial* mat, const aiTextureType type);
    };

    struct ModelResInfo
    {
    public:
        std::string                          file_path;
        std::vector<PerMaterialTexturePaths> per_material_texture_paths;

        ModelResInfo(const std::string& _file_path);
    };
} // namespace Meow
