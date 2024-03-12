#include "model.h"

#include "core/math/assimp_glm_helper.h"
#include "function/global/runtime_global_context.h"
#include "vertex_attribute.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glm/gtc/random.hpp>
#include <glm/gtx/quaternion.hpp>

#include <format>
#include <limits>
#include <unordered_map>

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
        this->attributes = attributes;

        int assimpFlags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_PreTransformVertices;

        for (size_t i = 0; i < attributes.size(); ++i)
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
            else if (attributes[i] == VertexAttribute::VA_SkinIndex)
            {
                loadSkin = true;
            }
            else if (attributes[i] == VertexAttribute::VA_SkinWeight)
            {
                loadSkin = true;
            }
            else if (attributes[i] == VertexAttribute::VA_SkinPack)
            {
                loadSkin = true;
            }
        }

        auto [data_ptr, data_size] = g_runtime_global_context.file_system.get()->ReadBinaryFile(file_path);

        Assimp::Importer importer;
        const aiScene*   scene = importer.ReadFileFromMemory((void*)data_ptr, data_size, assimpFlags);

        delete[] data_ptr;

        LoadBones(scene);
        LoadNode(physical_device, device, command_pool, queue, scene->mRootNode, scene);
        LoadAnim(scene);
    }

    void SimplifyTexturePath(std::string& path)
    {
        const size_t lastSlashIdx = path.find_last_of("\\/");
        if (std::string::npos != lastSlashIdx)
        {
            path.erase(0, lastSlashIdx + 1);
        }

        const size_t periodIdx = path.rfind('.');
        if (std::string::npos != periodIdx)
        {
            path.erase(periodIdx);
        }
    }

    void FillMaterialTextures(aiMaterial* ai_material, MaterialInfo& material)
    {
        if (ai_material->GetTextureCount(aiTextureType::aiTextureType_DIFFUSE))
        {
            aiString texture_path;
            ai_material->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &texture_path);
            material.diffuse = texture_path.C_Str();
            SimplifyTexturePath(material.diffuse);
        }

        if (ai_material->GetTextureCount(aiTextureType::aiTextureType_NORMALS))
        {
            aiString texture_path;
            ai_material->GetTexture(aiTextureType::aiTextureType_NORMALS, 0, &texture_path);
            material.normalmap = texture_path.C_Str();
            SimplifyTexturePath(material.normalmap);
        }

        if (ai_material->GetTextureCount(aiTextureType::aiTextureType_SPECULAR))
        {
            aiString texture_path;
            ai_material->GetTexture(aiTextureType::aiTextureType_SPECULAR, 0, &texture_path);
            material.specular = texture_path.C_Str();
            SimplifyTexturePath(material.specular);
        }
    }

    void Model::LoadBones(const aiScene* ai_scene)
    {
        std::unordered_map<std::string, size_t> bone_index_map;
        for (size_t i = 0; i < (size_t)ai_scene->mNumMeshes; ++i)
        {
            aiMesh* ai_mesh = ai_scene->mMeshes[i];
            for (size_t j = 0; j < (size_t)ai_mesh->mNumBones; ++j)
            {
                aiBone*     ai_bone = ai_mesh->mBones[j];
                std::string name    = ai_bone->mName.C_Str();

                auto it = bone_index_map.find(name);
                if (it == bone_index_map.end())
                {
                    // new bone
                    size_t     index        = (size_t)bones.size();
                    ModelBone* bone         = new ModelBone();
                    bone->index             = index;
                    bone->parent            = -1;
                    bone->name              = name;
                    bone->inverse_bind_pose = AssimpGLMHelpers::ConvertMatrixToGLMFormat(ai_bone->mOffsetMatrix);
                    // 记录Bone信息
                    bones.push_back(bone);
                    bones_map.insert(std::make_pair(name, bone));
                    // cache
                    bone_index_map.insert(std::make_pair(name, index));
                }
            }
        }
    }

    void Model::LoadSkin(std::unordered_map<size_t, ModelVertexSkin>& skin_info_map,
                         ModelMesh*                                   mesh,
                         const aiMesh*                                ai_mesh,
                         const aiScene*                               ai_scene)
    {
        std::unordered_map<size_t, size_t> bone_index_map;

        for (size_t i = 0; i < (size_t)ai_mesh->mNumBones; ++i)
        {
            aiBone*     bone_info = ai_mesh->mBones[i];
            std::string bone_name(bone_info->mName.C_Str());
            size_t      bone_index = bones_map[bone_name]->index;

            // bone在mesh中的索引
            size_t mesh_bone_index = 0;
            auto   it              = bone_index_map.find(bone_index);
            if (it == bone_index_map.end())
            {
                mesh_bone_index = (size_t)mesh->bones.size();
                mesh->bones.push_back(bone_index);
                bone_index_map.insert(std::make_pair(bone_index, mesh_bone_index));
            }
            else
            {
                mesh_bone_index = it->second;
            }

            // 收集被Bone影响的顶点信息
            for (size_t j = 0; j < bone_info->mNumWeights; ++j)
            {
                size_t vertexID = bone_info->mWeights[j].mVertexId;
                float  weight   = bone_info->mWeights[j].mWeight;
                // 顶点->Bone
                if (skin_info_map.find(vertexID) == skin_info_map.end())
                {
                    skin_info_map.insert(std::make_pair(vertexID, ModelVertexSkin()));
                }
                ModelVertexSkin* info     = &(skin_info_map[vertexID]);
                info->indices[info->used] = mesh_bone_index;
                info->weights[info->used] = weight;
                info->used += 1;
                // 只允许最多四个骨骼影响顶点
                if (info->used >= 4)
                {
                    break;
                }
            }
        }

        // 再次处理一遍skin_info_map，把未使用的补齐
        for (auto it = skin_info_map.begin(); it != skin_info_map.end(); ++it)
        {
            ModelVertexSkin& info = it->second;
            for (size_t i = info.used; i < 4; ++i)
            {
                info.indices[i] = 0;
                info.weights[i] = 0.0f;
            }
        }

        mesh->isSkin = true;
    }

    void Model::LoadVertexDatas(std::unordered_map<size_t, ModelVertexSkin>& skin_info_map,
                                std::vector<float>&                          vertices,
                                glm::vec3&                                   mmax,
                                glm::vec3&                                   mmin,
                                ModelMesh*                                   mesh,
                                const aiMesh*                                ai_mesh,
                                const aiScene*                               ai_scene)
    {
        glm::vec3 defaultColor(glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f));

        for (size_t i = 0; i < (size_t)ai_mesh->mNumVertices; ++i)
        {
            for (size_t j = 0; j < attributes.size(); ++j)
            {
                if (attributes[j] == VertexAttribute::VA_Position)
                {
                    float v0 = ai_mesh->mVertices[i].x;
                    float v1 = ai_mesh->mVertices[i].y;
                    float v2 = ai_mesh->mVertices[i].z;

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
                    if (ai_mesh->HasTextureCoords(0))
                    {
                        vertices.push_back(ai_mesh->mTextureCoords[0][i].x);
                        vertices.push_back(ai_mesh->mTextureCoords[0][i].y);
                    }
                    else
                    {
                        vertices.push_back(0);
                        vertices.push_back(0);
                    }
                }
                else if (attributes[j] == VertexAttribute::VA_UV1)
                {
                    if (ai_mesh->HasTextureCoords(1))
                    {
                        vertices.push_back(ai_mesh->mTextureCoords[1][i].x);
                        vertices.push_back(ai_mesh->mTextureCoords[1][i].y);
                    }
                    else
                    {
                        vertices.push_back(0);
                        vertices.push_back(0);
                    }
                }
                else if (attributes[j] == VertexAttribute::VA_Normal)
                {
                    vertices.push_back(ai_mesh->mNormals[i].x);
                    vertices.push_back(ai_mesh->mNormals[i].y);
                    vertices.push_back(ai_mesh->mNormals[i].z);
                }
                else if (attributes[j] == VertexAttribute::VA_Tangent)
                {
                    vertices.push_back(ai_mesh->mTangents[i].x);
                    vertices.push_back(ai_mesh->mTangents[i].y);
                    vertices.push_back(ai_mesh->mTangents[i].z);
                    vertices.push_back(1);
                }
                else if (attributes[j] == VertexAttribute::VA_Color)
                {
                    if (ai_mesh->HasVertexColors(i))
                    {
                        vertices.push_back(ai_mesh->mColors[0][i].r);
                        vertices.push_back(ai_mesh->mColors[0][i].g);
                        vertices.push_back(ai_mesh->mColors[0][i].b);
                    }
                    else
                    {
                        vertices.push_back(defaultColor.x);
                        vertices.push_back(defaultColor.y);
                        vertices.push_back(defaultColor.z);
                    }
                }
                else if (attributes[j] == VertexAttribute::VA_SkinPack)
                {
                    if (mesh->isSkin)
                    {
                        ModelVertexSkin& skin = skin_info_map[i];

                        size_t idx0       = skin.indices[0];
                        size_t idx1       = skin.indices[1];
                        size_t idx2       = skin.indices[2];
                        size_t idx3       = skin.indices[3];
                        size_t pack_index = (idx0 << 24) + (idx1 << 16) + (idx2 << 8) + idx3;

                        uint16_t weight0      = uint16_t(skin.weights[0] * 65535);
                        uint16_t weight1      = uint16_t(skin.weights[1] * 65535);
                        uint16_t weight2      = uint16_t(skin.weights[2] * 65535);
                        uint16_t weight3      = uint16_t(skin.weights[3] * 65535);
                        size_t   pack_weight0 = (weight0 << 16) + weight1;
                        size_t   pack_weight1 = (weight2 << 16) + weight3;

                        vertices.push_back((float)pack_index);
                        vertices.push_back((float)pack_weight0);
                        vertices.push_back((float)pack_weight1);
                    }
                    else
                    {
                        vertices.push_back(0);
                        vertices.push_back(65535);
                        vertices.push_back(0);
                    }
                }
                else if (attributes[j] == VertexAttribute::VA_SkinIndex)
                {
                    if (mesh->isSkin)
                    {
                        ModelVertexSkin& skin = skin_info_map[i];
                        vertices.push_back((float)skin.indices[0]);
                        vertices.push_back((float)skin.indices[1]);
                        vertices.push_back((float)skin.indices[2]);
                        vertices.push_back((float)skin.indices[3]);
                    }
                    else
                    {
                        vertices.push_back(0);
                        vertices.push_back(0);
                        vertices.push_back(0);
                        vertices.push_back(0);
                    }
                }
                else if (attributes[j] == VertexAttribute::VA_SkinWeight)
                {
                    if (mesh->isSkin)
                    {
                        ModelVertexSkin& skin = skin_info_map[i];
                        vertices.push_back(skin.weights[0]);
                        vertices.push_back(skin.weights[1]);
                        vertices.push_back(skin.weights[2]);
                        vertices.push_back(skin.weights[3]);
                    }
                    else
                    {
                        vertices.push_back(1.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(0.0f);
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
    }

    void Model::LoadIndices(std::vector<size_t>& indices, const aiMesh* ai_mesh, const aiScene* ai_scene)
    {
        for (size_t i = 0; i < (size_t)ai_mesh->mNumFaces; ++i)
        {
            indices.push_back(ai_mesh->mFaces[i].mIndices[0]);
            indices.push_back(ai_mesh->mFaces[i].mIndices[1]);
            indices.push_back(ai_mesh->mFaces[i].mIndices[2]);
        }
    }

    void Model::LoadPrimitives(vk::raii::PhysicalDevice const& physical_device,
                               vk::raii::Device const&         device,
                               vk::raii::CommandPool const&    command_pool,
                               vk::raii::Queue const&          queue,
                               std::vector<float>&             vertices,
                               std::vector<size_t>&            indices,
                               ModelMesh*                      mesh,
                               const aiMesh*                   aiMesh,
                               const aiScene*                  ai_scene)
    {
        size_t stride = (size_t)vertices.size() / aiMesh->mNumVertices;

        if (indices.size() > 65535)
        {
            std::unordered_map<size_t, size_t> indices_map;
            ModelPrimitive*                    primitive = nullptr;

            for (size_t i = 0; i < indices.size(); ++i)
            {
                size_t idx = indices[i];
                if (primitive == nullptr)
                {
                    primitive = new ModelPrimitive();
                    indices_map.clear();
                    mesh->primitives.push_back(primitive);
                }

                size_t newIdx = 0;
                auto   it     = indices_map.find(idx);
                if (it == indices_map.end())
                {
                    size_t start = idx * stride;
                    newIdx       = (size_t)primitive->vertices.size() / stride;
                    primitive->vertices.insert(
                        primitive->vertices.end(), vertices.begin() + start, vertices.begin() + start + stride);
                    indices_map.insert(std::make_pair(idx, newIdx));
                }
                else
                {
                    newIdx = it->second;
                }

                primitive->indices.push_back(newIdx);

                if (primitive->indices.size() == 65535)
                {
                    primitive = nullptr;
                }
            }

            for (size_t i = 0; i < mesh->primitives.size(); ++i)
            {
                primitive                    = mesh->primitives[i];
                primitive->vertex_buffer_ptr = std::make_shared<VertexBuffer>(physical_device,
                                                                              device,
                                                                              command_pool,
                                                                              queue,
                                                                              vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                                              primitive->vertices);
                primitive->index_buffer_ptr  = std::make_shared<IndexBuffer>(physical_device,
                                                                            device,
                                                                            command_pool,
                                                                            queue,
                                                                            vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                                            primitive->indices);
            }
        }
        else
        {
            ModelPrimitive* primitive = new ModelPrimitive();
            primitive->vertices       = vertices;
            for (uint16_t i = 0; i < indices.size(); ++i)
            {
                primitive->indices.push_back(indices[i]);
            }
            mesh->primitives.push_back(primitive);

            primitive->vertex_buffer_ptr = std::make_shared<VertexBuffer>(physical_device,
                                                                          device,
                                                                          command_pool,
                                                                          queue,
                                                                          vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                                          primitive->vertices);
            primitive->index_buffer_ptr  = std::make_shared<IndexBuffer>(physical_device,
                                                                        device,
                                                                        command_pool,
                                                                        queue,
                                                                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                                        primitive->indices);
        }

        for (size_t i = 0; i < mesh->primitives.size(); ++i)
        {
            ModelPrimitive* primitive = mesh->primitives[i];
            primitive->vertex_count   = (size_t)primitive->vertices.size() / stride;
            primitive->triangle_num   = (size_t)primitive->indices.size() / 3;

            mesh->vertex_count += primitive->vertex_count;
            mesh->triangle_count += primitive->triangle_num;
        }
    }

    ModelMesh* Model::LoadMesh(vk::raii::PhysicalDevice const& physical_device,
                               vk::raii::Device const&         device,
                               vk::raii::CommandPool const&    command_pool,
                               vk::raii::Queue const&          queue,
                               const aiMesh*                   ai_mesh,
                               const aiScene*                  ai_scene)
    {
        ModelMesh* mesh = new ModelMesh();

        // load material
        aiMaterial* material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
        if (material)
        {
            FillMaterialTextures(material, mesh->material);
        }

        // load bones
        std::unordered_map<size_t, ModelVertexSkin> skin_info_map;
        if (ai_mesh->mNumBones > 0 && loadSkin)
        {
            LoadSkin(skin_info_map, mesh, ai_mesh, ai_scene);
        }

        // load vertex data
        std::vector<float> vertices;
        glm::vec3          mmin(
            std::numeric_limits<float>().max(), std::numeric_limits<float>().max(), std::numeric_limits<float>().max());
        glm::vec3 mmax(-std::numeric_limits<float>().max(),
                       -std::numeric_limits<float>().max(),
                       -std::numeric_limits<float>().max());
        LoadVertexDatas(skin_info_map, vertices, mmax, mmin, mesh, ai_mesh, ai_scene);

        // load indices
        std::vector<size_t> indices;
        LoadIndices(indices, ai_mesh, ai_scene);

        // load primitives
        LoadPrimitives(physical_device, device, command_pool, queue, vertices, indices, mesh, ai_mesh, ai_scene);

        mesh->bounding.min = mmin;
        mesh->bounding.max = mmax;
        mesh->bounding.UpdateCorners();

        return mesh;
    }

    ModelNode* Model::LoadNode(vk::raii::PhysicalDevice const& physical_device,
                               vk::raii::Device const&         device,
                               vk::raii::CommandPool const&    command_pool,
                               vk::raii::Queue const&          queue,
                               const aiNode*                   aiNode,
                               const aiScene*                  ai_scene)
    {
        ModelNode* model_node = new ModelNode();
        model_node->name      = aiNode->mName.C_Str();

        if (root_node == nullptr)
        {
            root_node = model_node;
        }

        // local matrix
        model_node->local_matrix = AssimpGLMHelpers::ConvertMatrixToGLMFormat(aiNode->mTransformation);
        // mesh
        if (aiNode->mNumMeshes > 0)
        {
            for (size_t i = 0; i < aiNode->mNumMeshes; ++i)
            {
                ModelMesh* vkMesh = LoadMesh(
                    physical_device, device, command_pool, queue, ai_scene->mMeshes[aiNode->mMeshes[i]], ai_scene);
                vkMesh->link_node = model_node;
                model_node->meshes.push_back(vkMesh);
                meshes.push_back(vkMesh);
            }
        }

        // nodes map
        nodes_map.insert(std::make_pair(model_node->name, model_node));
        linear_nodes.push_back(model_node);

        // bones parent
        size_t boneParentIndex = -1;
        {
            auto it = bones_map.find(model_node->name);
            if (it != bones_map.end())
            {
                boneParentIndex = it->second->index;
            }
        }

        // children node
        for (size_t i = 0; i < (size_t)aiNode->mNumChildren; ++i)
        {
            ModelNode* childNode =
                LoadNode(physical_device, device, command_pool, queue, aiNode->mChildren[i], ai_scene);
            childNode->parent = model_node;
            model_node->children.push_back(childNode);

            // bones relationship
            {
                auto it = bones_map.find(childNode->name);
                if (it != bones_map.end())
                {
                    it->second->parent = boneParentIndex;
                }
            }
        }

        return model_node;
    }

    void Model::LoadAnim(const aiScene* ai_scene)
    {
        for (size_t i = 0; i < (size_t)ai_scene->mNumAnimations; ++i)
        {
            aiAnimation* ai_animation = ai_scene->mAnimations[i];
            float        timeTick = ai_animation->mTicksPerSecond != 0 ? (float)ai_animation->mTicksPerSecond : 25.0f;

            animations.push_back(ModelAnimation());
            ModelAnimation& model_animation = animations.back();

            for (size_t j = 0; j < (size_t)ai_animation->mNumChannels; ++j)
            {
                aiNodeAnim* nodeAnim  = ai_animation->mChannels[j];
                std::string node_name = nodeAnim->mNodeName.C_Str();

                model_animation.clips.insert(std::make_pair(node_name, ModelAnimationClip()));

                ModelAnimationClip& animClip = model_animation.clips[node_name];
                animClip.node_name           = node_name;
                animClip.duration            = 0.0f;

                // position
                for (size_t index = 0; index < (size_t)nodeAnim->mNumPositionKeys; ++index)
                {
                    aiVectorKey& aikey = nodeAnim->mPositionKeys[index];
                    animClip.positions.keys.push_back((float)aikey.mTime / timeTick);
                    animClip.positions.values.push_back(glm::vec3(aikey.mValue.x, aikey.mValue.y, aikey.mValue.z));
                    animClip.duration = glm::max((float)aikey.mTime / timeTick, animClip.duration);
                }

                // scale
                for (size_t index = 0; index < (size_t)nodeAnim->mNumScalingKeys; ++index)
                {
                    aiVectorKey& aikey = nodeAnim->mScalingKeys[index];
                    animClip.scales.keys.push_back((float)aikey.mTime / timeTick);
                    animClip.scales.values.push_back(glm::vec3(aikey.mValue.x, aikey.mValue.y, aikey.mValue.z));
                    animClip.duration = glm::max((float)aikey.mTime / timeTick, animClip.duration);
                }

                // rotation
                for (size_t index = 0; index < (size_t)nodeAnim->mNumRotationKeys; ++index)
                {
                    aiQuatKey& aikey = nodeAnim->mRotationKeys[index];
                    animClip.rotations.keys.push_back((float)aikey.mTime / timeTick);
                    animClip.rotations.values.push_back(
                        glm::quat(aikey.mValue.x, aikey.mValue.y, aikey.mValue.z, aikey.mValue.w));
                    animClip.duration = glm::max((float)aikey.mTime / timeTick, animClip.duration);
                }

                model_animation.duration = glm::max(animClip.duration, model_animation.duration);
            }
        }
    }

    void Model::GotoAnimation(float time)
    {
        if (animIndex == -1)
        {
            return;
        }

        ModelAnimation& animation = animations[animIndex];
        animation.time            = glm::clamp(time, 0.0f, animation.duration);

        // update nodes animation
        for (auto it = animation.clips.begin(); it != animation.clips.end(); ++it)
        {
            ModelAnimationClip& clip = it->second;
            ModelNode*          node = nodes_map[clip.node_name];

            float alpha = 0.0f;

            // rotation
            glm::quat prevRot(0, 0, 0, 1);
            glm::quat nextRot(0, 0, 0, 1);
            clip.rotations.GetValue(animation.time, prevRot, nextRot, alpha);
            glm::quat retRot = glm::lerp(prevRot, nextRot, alpha);

            // position
            glm::vec3 prevPos(0, 0, 0);
            glm::vec3 nextPos(0, 0, 0);
            clip.positions.GetValue(animation.time, prevPos, nextPos, alpha);
            glm::vec3 retPos = glm::mix(prevPos, nextPos, alpha);

            // scale
            glm::vec3 prevScale(1, 1, 1);
            glm::vec3 nextScale(1, 1, 1);
            clip.scales.GetValue(animation.time, prevScale, nextScale, alpha);
            glm::vec3 retScale = glm::mix(prevScale, nextScale, alpha);

            node->local_matrix = glm::mat4(1.0f);
            node->local_matrix = glm::scale(node->local_matrix, retScale);
            node->local_matrix = glm::toMat4(retRot) * node->local_matrix;
            node->local_matrix = glm::translate(node->local_matrix, retPos);
        }

        // update bones
        for (size_t i = 0; i < bones.size(); ++i)
        {
            ModelBone* bone = bones[i];
            ModelNode* node = nodes_map[bone->name];
            // 注意行列矩阵的区别
            bone->final_transform = bone->inverse_bind_pose;
            bone->final_transform = node->GetGlobalMatrix() * bone->final_transform;
        }
    }

    void Model::Update(float time, float delta)
    {
        if (animIndex == -1)
        {
            return;
        }

        ModelAnimation& animation = animations[animIndex];
        animation.time += delta * animation.speed;

        if (animation.time >= animation.duration)
        {
            animation.time = animation.time - animation.duration;
        }

        GotoAnimation(animation.time);
    }

    void Model::SetAnimation(size_t index)
    {
        if (index >= animations.size())
        {
            return;
        }
        if (index < 0)
        {
            return;
        }
        animIndex = index;
    }

    ModelAnimation& Model::GetAnimation(size_t index)
    {
        if (index == -1)
        {
            index = animIndex;
        }
        return animations[index];
    }

} // namespace Meow
