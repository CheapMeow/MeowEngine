#include "model.h"

#include "core/math/assimp_glm_helper.h"
#include "function/global/runtime_global_context.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <limits>

namespace Meow
{
    /**
     * @brief Current input binding only bind to 0, it may be changed when app need to solve complex situation.
     */
    VkVertexInputBindingDescription Model::GetInputBinding()
    {
        int32_t stride = 0;
        for (int32_t i = 0; i < m_attributes.size(); ++i)
        {
            stride += VertexAttributeToSize(m_attributes[i]);
        }

        VkVertexInputBindingDescription vertex_input_binding = {};
        vertex_input_binding.binding                         = 0;
        vertex_input_binding.stride                          = stride;
        vertex_input_binding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

        return vertex_input_binding;
    }

    /**
     * @brief Current input binding only bind to 0, it may be changed when app need to solve complex situation.
     */
    std::vector<VkVertexInputAttributeDescription> Model::GetInputAttributes()
    {
        std::vector<VkVertexInputAttributeDescription> vertex_input_attributes;
        int32_t                                        offset = 0;

        for (int32_t i = 0; i < m_attributes.size(); ++i)
        {
            VkVertexInputAttributeDescription input_attributes = {};
            input_attributes.binding                           = 0;
            input_attributes.location                          = i;
            input_attributes.format                            = VertexAttributeToVkFormat(m_attributes[i]);
            input_attributes.offset                            = offset;
            offset += VertexAttributeToSize(m_attributes[i]);
            vertex_input_attributes.push_back(input_attributes);
        }

        return vertex_input_attributes;
    }

    void Model::Draw(const vk::raii::CommandBuffer& cmd_buffer)
    {
        for (const auto& meshes_per_material : m_meshes_per_material_map)
        {
            const uint32_t&          material_index = meshes_per_material.first;
            const std::vector<Mesh>& meshes         = meshes_per_material.second;

            for (auto& mesh : meshes)
            {
                mesh.BindDrawCmd(cmd_buffer);
            }
        }
    }

    std::vector<uint32_t> Model::LoadMaterialTextures(const aiMaterial* mat, const aiTextureType type)
    {
        std::vector<uint32_t> texture_indices;
        for (uint32_t i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, 0, &str);
            bool        skip         = false;
            std::string texture_path = m_directory + str.C_Str();
            for (uint32_t j = 0; j < m_texture_paths.size(); j++)
            {
                if (m_texture_paths[j] == texture_path)
                {
                    texture_indices.push_back(j);
                    skip = true;
                    break;
                }
            }
            if (!skip)
            {
                texture_indices.push_back(m_texture_paths.size());
                m_texture_paths.push_back(texture_path);
            }
        }
        return texture_indices;
    }

    /**
     * @brief Load model from file using assimp.
     *
     * Use aiProcess_PreTransformVertices when importing, so model node doesn't need to save local transform matrix.
     *
     * If you keep local transform matrix of model node, it means you should create uniform buffer for each model node.
     * Then when draw a mesh once you should update buffer data once.
     */
    void Model::LoadFromFile(vk::raii::PhysicalDevice const& physical_device,
                             vk::raii::Device const&         device,
                             const std::string&              file_name)
    {
        int assimpFlags = aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_FlipUVs |
                          aiProcess_FlipWindingOrder | aiProcess_PreTransformVertices;

        for (int32_t i = 0; i < m_attributes.size(); ++i)
        {
            if (m_attributes[i] == VertexAttribute::VA_Tangent)
            {
                assimpFlags = assimpFlags | aiProcess_CalcTangentSpace;
            }
            else if (m_attributes[i] == VertexAttribute::VA_UV0)
            {
                assimpFlags = assimpFlags | aiProcess_GenUVCoords;
            }
            else if (m_attributes[i] == VertexAttribute::VA_Normal)
            {
                assimpFlags = assimpFlags | aiProcess_GenSmoothNormals;
            }
        }

        uint8_t* data_ptr;
        uint32_t data_size;
        g_runtime_global_context.m_file_system.get()->ReadBinaryFile(file_name, data_ptr, data_size);

        Assimp::Importer importer;
        const aiScene*   scene = importer.ReadFileFromMemory((void*)data_ptr, data_size, assimpFlags);

        LoadNode(physical_device, device, scene->mRootNode, scene);

        delete[] data_ptr;
    }

    void Model::LoadNode(vk::raii::PhysicalDevice const& physical_device,
                         vk::raii::Device const&         device,
                         const aiNode*                   aiNode,
                         const aiScene*                  aiScene)
    {
        // mesh
        for (int i = 0; i < aiNode->mNumMeshes; ++i)
        {
            LoadMesh(physical_device, device, aiScene->mMeshes[aiNode->mMeshes[i]], aiScene);
        }

        // children node
        for (int32_t i = 0; i < aiNode->mNumChildren; ++i)
        {
            LoadNode(physical_device, device, aiNode->mChildren[i], aiScene);
        }
    }

    void Model::LoadMesh(vk::raii::PhysicalDevice const& physical_device,
                         vk::raii::Device const&         device,
                         const aiMesh*                   aiMesh,
                         const aiScene*                  aiScene)
    {
        glm::vec3 mmin(
            std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        glm::vec3 mmax(
            std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

        std::vector<float>    vertices;
        std::vector<uint32_t> indices;

        uint32_t    material_index = 0;
        aiMaterial* material       = aiScene->mMaterials[aiMesh->mMaterialIndex];
        if (material)
        {
            std::vector<uint32_t> diffuse_texture_indices =
                LoadMaterialTextures(material, aiTextureType::aiTextureType_DIFFUSE);
            std::vector<uint32_t> normal_texture_indices =
                LoadMaterialTextures(material, aiTextureType::aiTextureType_NORMALS);
            std::vector<uint32_t> specular_texture_indices =
                LoadMaterialTextures(material, aiTextureType::aiTextureType_SPECULAR);

            uint32_t diffuse_texture_index =
                diffuse_texture_indices.size() > 0 ? diffuse_texture_indices[0] : std::numeric_limits<uint32_t>::max();
            uint32_t normal_texture_index =
                normal_texture_indices.size() > 0 ? normal_texture_indices[0] : std::numeric_limits<uint32_t>::max();
            uint32_t specular_texture_index = specular_texture_indices.size() > 0 ?
                                                  specular_texture_indices[0] :
                                                  std::numeric_limits<uint32_t>::max();

            MaterialInfo material_info(diffuse_texture_index, normal_texture_index, specular_texture_index);

            bool skip = false;
            for (uint32_t i = 0; i < m_material_infos.size(); i++)
            {
                if (CompareMaterial(m_material_infos[i], material_info))
                {
                    material_index = i;
                    skip           = true;
                    break;
                }
            }
            if (!skip)
            {
                material_index = m_material_infos.size();
                m_material_infos.push_back(material_info);
            }
        }

        aiString texPath;
        material->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &texPath);

        for (int32_t i = 0; i < aiMesh->mNumVertices; ++i)
        {
            for (int32_t j = 0; j < m_attributes.size(); ++j)
            {
                if (m_attributes[j] == VertexAttribute::VA_Position)
                {
                    float v0 = aiMesh->mVertices[i].x;
                    float v1 = aiMesh->mVertices[i].y;
                    float v2 = aiMesh->mVertices[i].z;

                    vertices.push_back(v0);
                    vertices.push_back(v1);
                    vertices.push_back(v2);

                    mmin.x = glm::min(v0, mmin.x);
                    mmin.y = glm::min(v1, mmin.y);
                    mmin.z = glm::min(v2, mmin.z);
                    mmax.x = glm::max(v0, mmax.x);
                    mmax.y = glm::max(v1, mmax.y);
                    mmax.z = glm::max(v2, mmax.z);
                }
                else if (m_attributes[j] == VertexAttribute::VA_UV0)
                {
                    vertices.push_back(aiMesh->mTextureCoords[0][i].x);
                    vertices.push_back(aiMesh->mTextureCoords[0][i].y);
                }
                else if (m_attributes[j] == VertexAttribute::VA_UV1)
                {
                    vertices.push_back(aiMesh->mTextureCoords[1][i].x);
                    vertices.push_back(aiMesh->mTextureCoords[1][i].y);
                }
                else if (m_attributes[j] == VertexAttribute::VA_Normal)
                {
                    vertices.push_back(aiMesh->mNormals[i].x);
                    vertices.push_back(aiMesh->mNormals[i].y);
                    vertices.push_back(aiMesh->mNormals[i].z);
                }
                else if (m_attributes[j] == VertexAttribute::VA_Tangent)
                {
                    vertices.push_back(aiMesh->mTangents[i].x);
                    vertices.push_back(aiMesh->mTangents[i].y);
                    vertices.push_back(aiMesh->mTangents[i].z);
                    vertices.push_back(1);
                }
                else if (m_attributes[j] == VertexAttribute::VA_Color)
                {
                    if (aiMesh->HasVertexColors(i))
                    {
                        vertices.push_back(aiMesh->mColors[0][i].r);
                        vertices.push_back(aiMesh->mColors[0][i].g);
                        vertices.push_back(aiMesh->mColors[0][i].b);
                    }
                    else
                    {
                        // default vertex color
                        vertices.push_back(1.0f);
                        vertices.push_back(1.0f);
                        vertices.push_back(1.0f);
                    }
                }
                else if (m_attributes[j] == VertexAttribute::VA_Custom0 ||
                         m_attributes[j] == VertexAttribute::VA_Custom1 ||
                         m_attributes[j] == VertexAttribute::VA_Custom2 ||
                         m_attributes[j] == VertexAttribute::VA_Custom3)
                {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
            }
        }

        for (int32_t i = 0; i < aiMesh->mNumFaces; ++i)
        {
            indices.push_back(aiMesh->mFaces[i].mIndices[0]);
            indices.push_back(aiMesh->mFaces[i].mIndices[1]);
            indices.push_back(aiMesh->mFaces[i].mIndices[2]);
        }

        std::vector<Primitive> primitives;

        int32_t stride = vertices.size() / aiMesh->mNumVertices;

        // when vertex count > 65535 in one mesh, spilt it
        // because some device only support uint16_t indices
        // uint16_t means max count is 65535
        if (indices.size() > 65535)
        {
            std::unordered_map<uint16_t, uint16_t> indices_map;
            for (size_t _ = 0; _ <= indices.size() / 65535; _++)
            {
                std::vector<float>    primitive_vertices;
                std::vector<uint16_t> primitive_indices;

                for (int32_t i = 0; i < indices.size(); ++i)
                {
                    indices_map.clear();

                    uint16_t idx = indices[i];

                    uint16_t new_index = 0;
                    auto     it        = indices_map.find(idx);
                    if (it == indices_map.end())
                    {
                        uint16_t start = idx * stride;
                        // new index is linear growing
                        new_index = primitive_vertices.size() / stride;
                        // find and copy vertex correspond to the index
                        primitive_vertices.insert(
                            primitive_vertices.end(), vertices.begin() + start, vertices.begin() + start + stride);
                        // old index - new index
                        indices_map.insert(std::make_pair(idx, new_index));
                    }
                    else
                    {
                        new_index = it->second;
                    }

                    primitive_indices.push_back(new_index);
                }

                // TODO: Support uint32_t indices
                primitives.emplace_back(std::move(primitive_vertices),
                                        std::move(primitive_indices),
                                        physical_device,
                                        device,
                                        vk::MemoryPropertyFlagBits::eHostVisible |
                                            vk::MemoryPropertyFlagBits::eHostCoherent,
                                        m_attributes,
                                        m_index_type);
            }
        }
        else
        {
            std::vector<uint16_t> primitive_indices;

            for (auto& index : indices)
            {
                primitive_indices.push_back(index);
            }

            primitives.emplace_back(std::move(vertices),
                                    std::move(primitive_indices),
                                    physical_device,
                                    device,
                                    vk::MemoryPropertyFlagBits::eHostVisible |
                                        vk::MemoryPropertyFlagBits::eHostCoherent,
                                    m_attributes,
                                    m_index_type);
        }

        m_meshes_per_material_map[material_index].emplace_back(std::move(primitives), BoundingBox(mmin, mmax));
        m_bounding.Merge(mmin, mmax);
    }
} // namespace Meow
