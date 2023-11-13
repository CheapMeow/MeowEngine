#include "mesh.h"

#include <unordered_map>

namespace Meow
{
    Mesh::Mesh(vk::raii::PhysicalDevice const& physical_device,
               vk::raii::Device const&         device,
               vk::raii::CommandPool const&    command_pool,
               vk::raii::Queue const&          queue,
               const aiMesh*                   aiMesh,
               std::vector<VertexAttribute>    attributes,
               vk::IndexType                   index_type)
    {
        glm::vec3 mmin(
            std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        glm::vec3 mmax(
            std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

        std::vector<float>    vertices;
        std::vector<uint32_t> indices;

        for (int32_t i = 0; i < aiMesh->mNumVertices; ++i)
        {
            for (int32_t j = 0; j < attributes.size(); ++j)
            {
                if (attributes[j] == VertexAttribute::VA_Position)
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
                else if (attributes[j] == VertexAttribute::VA_UV0)
                {
                    vertices.push_back(aiMesh->mTextureCoords[0][i].x);
                    vertices.push_back(aiMesh->mTextureCoords[0][i].y);
                }
                else if (attributes[j] == VertexAttribute::VA_UV1)
                {
                    vertices.push_back(aiMesh->mTextureCoords[1][i].x);
                    vertices.push_back(aiMesh->mTextureCoords[1][i].y);
                }
                else if (attributes[j] == VertexAttribute::VA_Normal)
                {
                    vertices.push_back(aiMesh->mNormals[i].x);
                    vertices.push_back(aiMesh->mNormals[i].y);
                    vertices.push_back(aiMesh->mNormals[i].z);
                }
                else if (attributes[j] == VertexAttribute::VA_Tangent)
                {
                    vertices.push_back(aiMesh->mTangents[i].x);
                    vertices.push_back(aiMesh->mTangents[i].y);
                    vertices.push_back(aiMesh->mTangents[i].z);
                    vertices.push_back(1);
                }
                else if (attributes[j] == VertexAttribute::VA_Color)
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
                else if (attributes[j] == VertexAttribute::VA_Custom0 || attributes[j] == VertexAttribute::VA_Custom1 ||
                         attributes[j] == VertexAttribute::VA_Custom2 || attributes[j] == VertexAttribute::VA_Custom3)
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

        int32_t stride = vertices.size() / aiMesh->mNumVertices;

        // when vertex count > 65535 in one mesh, spilt it
        // because some device only support uint16_t indices
        // uint16_t means max count is 65535
        if (indices.size() > 65535)
        {
            std::unordered_map<uint16_t, uint16_t> indices_map;
            for (size_t primitive_index = 0; primitive_index <= indices.size() / 65535; ++primitive_index)
            {
                std::vector<float>    primitive_vertices;
                std::vector<uint16_t> primitive_indices;

                for (size_t i = 0; i < indices.size(); ++i)
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
                primitives.emplace_back(std::make_shared<Primitive>(
                    physical_device,
                    device,
                    command_pool,
                    queue,
                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                    index_type,
                    primitive_vertices,
                    primitive_indices,
                    primitive_vertices.size() / VertexAttributesToSize(attributes) * sizeof(float),
                    primitive_indices.size()
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
                        ,
                    std::format("{} {}", aiMesh->mName.C_Str(), primitive_index).c_str()
#endif
                        ));
            }
        }
        else
        {
            std::vector<uint16_t> primitive_indices;

            for (auto& index : indices)
            {
                primitive_indices.push_back(index);
            }

            primitives.emplace_back(
                std::make_shared<Primitive>(physical_device,
                                            device,
                                            command_pool,
                                            queue,
                                            vk::MemoryPropertyFlagBits::eDeviceLocal,
                                            index_type,
                                            vertices,
                                            primitive_indices,
                                            vertices.size() / VertexAttributesToSize(attributes) * sizeof(float),
                                            primitive_indices.size()
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
                                                ,
                                            aiMesh->mName.C_Str()
#endif
                                                ));
        }

        bounding = BoundingBox(mmin, mmax);
    }

    void Mesh::BindDrawCmd(vk::raii::CommandBuffer const& cmd_buffer) const
    {
        for (size_t i = 0; i < primitives.size(); ++i)
        {
            primitives[i]->BindDrawCmd(cmd_buffer);
        }
    }

} // namespace Meow