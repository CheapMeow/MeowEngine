#include "model.h"

#include "core/math/assimp_glm_helper.h"
#include "function/global/runtime_global_context.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <format>
#include <limits>

namespace Meow
{
    /**
     * @brief Load model from file using assimp.
     *
     * Use aiProcess_PreTransformVertices when importing, so model node doesn't need to save local transform matrix.
     *
     * If you keep local transform matrix of model node, it means you should create uniform buffer for each model node.
     * Then when draw a mesh once you should update buffer data once.
     */
    Model::Model(vk::raii::PhysicalDevice const& physical_device,
                 vk::raii::Device const&         device,
                 vk::raii::CommandPool const&    command_pool,
                 vk::raii::Queue const&          queue,
                 const std::string&              file_path,
                 std::vector<VertexAttribute>    attributes,
                 vk::IndexType                   index_type = vk::IndexType::eUint16)
    {
        int assimpFlags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_PreTransformVertices;

        for (int32_t i = 0; i < attributes.size(); ++i)
        {
            if (attributes[i] == VertexAttribute::VA_Tangent)
            {
                assimpFlags = assimpFlags | aiProcess_CalcTangentSpace;
            }
            else if (attributes[i] == VertexAttribute::VA_UV0)
            {
                assimpFlags = assimpFlags | aiProcess_GenUVCoords;
            }
            else if (attributes[i] == VertexAttribute::VA_Normal)
            {
                assimpFlags = assimpFlags | aiProcess_GenSmoothNormals;
            }
        }

        uint8_t* data_ptr;
        uint32_t data_size;
        g_runtime_global_context.file_system.get()->ReadBinaryFile(file_path, data_ptr, data_size);

        Assimp::Importer importer;
        const aiScene*   scene = importer.ReadFileFromMemory((void*)data_ptr, data_size, assimpFlags);

        delete[] data_ptr;

        for (size_t i = 0; i < scene->mNumMeshes; ++i)
        {
            meshes.emplace_back(
                physical_device, device, command_pool, queue, scene->mMeshes[i], attributes, index_type);

            bounding.Merge(meshes[i].bounding);
        }
    }

} // namespace Meow
