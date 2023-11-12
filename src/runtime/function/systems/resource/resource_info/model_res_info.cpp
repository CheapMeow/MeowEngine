#include "model_res_info.h"

#include "function/global/runtime_global_context.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace Meow
{
    std::vector<std::string> PerMaterialTexturePaths::LoadMaterialTextures(const std::string&  file_path,
                                                                           const aiMaterial*   mat,
                                                                           const aiTextureType type)
    {
        std::vector<std::string> texture_paths;
        for (uint32_t i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            std::filesystem::path texture_path(file_path);
            texture_path = texture_path / str.C_Str();
            texture_path = texture_path.lexically_normal();

            texture_paths.push_back(texture_path.string());
        }
        return texture_paths;
    }

    ModelResInfo::ModelResInfo(const std::string& _file_path)
        : file_path(_file_path)
    {
        int assimpFlags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_PreTransformVertices;

        uint8_t* data_ptr;
        uint32_t data_size;
        g_runtime_global_context.file_system.get()->ReadBinaryFile(file_path, data_ptr, data_size);

        Assimp::Importer importer;
        const aiScene*   scene = importer.ReadFileFromMemory((void*)data_ptr, data_size, assimpFlags);

        for (size_t i = 0; i < scene->mNumMaterials; ++i)
        {
            per_material_texture_paths.emplace_back(file_path, scene->mMaterials[i]);
        }
    }
} // namespace Meow
