#include "model.hpp"

#include "pch.h"

#include "core/math/assimp_glm_helper.h"
#include "function/global/runtime_context.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glm/gtc/random.hpp>
#include <glm/gtx/quaternion.hpp>

#include <format>
#include <limits>
#include <unordered_map>

namespace Meow
{
    Model::Model(const std::string& file_path, const std::vector<VertexAttributeBit>& attributes)
    {
        this->attributes = attributes;

        int assimpFlags = aiProcess_Triangulate | aiProcess_FlipUVs;

        for (size_t i = 0; i < attributes.size(); ++i)
        {
            if (attributes[i] == VertexAttributeBit::Tangent)
            {
                assimpFlags = assimpFlags | aiProcess_CalcTangentSpace;
            }
            if (attributes[i] == VertexAttributeBit::UV0)
            {
                assimpFlags = assimpFlags | aiProcess_GenUVCoords;
            }
            if (attributes[i] == VertexAttributeBit::Normal)
            {
                assimpFlags = assimpFlags | aiProcess_GenSmoothNormals;
            }
            if (attributes[i] == VertexAttributeBit::SkinIndex)
            {
                loadSkin = true;
            }
            if (attributes[i] == VertexAttributeBit::SkinWeight)
            {
                loadSkin = true;
            }
            if (attributes[i] == VertexAttributeBit::SkinPack)
            {
                loadSkin = true;
            }
        }

        Assimp::Importer importer;
        const aiScene*   scene =
            importer.ReadFile(g_runtime_context.file_system->GetAbsolutePath(file_path), assimpFlags);
        if (scene == nullptr)
        {
            MEOW_ERROR("Read model file {} failed!", file_path);
            return;
        }

        root_path = std::filesystem::path(g_runtime_context.file_system->GetAbsolutePath(file_path)).parent_path();

        LoadBones(scene);
        LoadNode(scene->mRootNode, scene);
        LoadAnim(scene);
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

    BoundingBox Model::GetBounding()
    {
        BoundingBox bounding;

        for (const auto& node : linear_nodes)
        {
            bounding.Merge(node->GetBounds());
        }

        return bounding;
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
            // caution the difference between row major and column major
            bone->final_transform = bone->inverse_bind_pose;
            bone->final_transform = node->GetGlobalMatrix() * bone->final_transform;
        }
    }

    ModelNode* Model::LoadNode(const aiNode* aiNode, const aiScene* ai_scene)
    {
        auto model_node  = new ModelNode();
        model_node->name = aiNode->mName.C_Str();

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
                ModelMesh* vkMesh = LoadMesh(ai_scene->mMeshes[aiNode->mMeshes[i]], ai_scene);
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
            ModelNode* childNode = LoadNode(aiNode->mChildren[i], ai_scene);
            childNode->parent    = model_node;
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

    ModelMesh* Model::LoadMesh(const aiMesh* ai_mesh, const aiScene* ai_scene)
    {
        auto mesh = new ModelMesh();

        // load material
        aiMaterial* material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
        if (material)
        {
            // TODO: load texture to resource system
            // FillMaterialTextures(material, mesh->texture_info);
        }

        // load bones
        std::unordered_map<size_t, ModelVertexSkin> skin_info_map;
        if (ai_mesh->mNumBones > 0 && loadSkin)
        {
            LoadSkin(skin_info_map, mesh, ai_mesh, ai_scene);
        }

        // load vertex data
        glm::vec3 mmin(
            std::numeric_limits<float>().max(), std::numeric_limits<float>().max(), std::numeric_limits<float>().max());
        glm::vec3 mmax(-std::numeric_limits<float>().max(),
                       -std::numeric_limits<float>().max(),
                       -std::numeric_limits<float>().max());

        LoadVertexDatas(skin_info_map, mesh->vertices, mmax, mmin, mesh, ai_mesh, ai_scene);

        mesh->bounding.min = mmin;
        mesh->bounding.max = mmax;
        mesh->bounding.UpdateCorners();

        // load indices
        LoadIndices(mesh->indices, ai_mesh, ai_scene);

        mesh->RefreshBuffer();
        mesh->vertex_count   = ai_mesh->mNumVertices;
        mesh->triangle_count = (size_t)mesh->indices.size() / 3;

        return mesh;
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
                    size_t index            = (size_t)bones.size();
                    auto   bone             = new ModelBone();
                    bone->index             = index;
                    bone->parent            = -1;
                    bone->name              = name;
                    bone->inverse_bind_pose = AssimpGLMHelpers::ConvertMatrixToGLMFormat(ai_bone->mOffsetMatrix);
                    // record bone info
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

            // collect the vertex influented by the bone
            for (size_t vert_idx = 0; vert_idx < bone_info->mNumWeights; ++vert_idx)
            {
                size_t vertexID = bone_info->mWeights[vert_idx].mVertexId;
                float  weight   = bone_info->mWeights[vert_idx].mWeight;
                // vertex -> Bone
                if (skin_info_map.find(vertexID) == skin_info_map.end())
                {
                    skin_info_map.insert(std::make_pair(vertexID, ModelVertexSkin()));
                }
                ModelVertexSkin* info     = &(skin_info_map[vertexID]);
                info->indices[info->used] = mesh_bone_index;
                info->weights[info->used] = weight;
                info->used += 1;
                // maximum number of bones to influent one vertex is 4
                if (info->used >= 4)
                {
                    break;
                }
            }
        }

        // process skin_info_map again, supplement the unused
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
            // according to binding order
            for (size_t j = 0; j < attributes.size(); ++j)
            {
                if (attributes[j] == VertexAttributeBit::Position)
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
                if (attributes[j] == VertexAttributeBit::UV0)
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
                if (attributes[j] == VertexAttributeBit::UV1)
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
                if (attributes[j] == VertexAttributeBit::Normal)
                {
                    vertices.push_back(ai_mesh->mNormals[i].x);
                    vertices.push_back(ai_mesh->mNormals[i].y);
                    vertices.push_back(ai_mesh->mNormals[i].z);
                }
                if (attributes[j] == VertexAttributeBit::Tangent)
                {
                    vertices.push_back(ai_mesh->mTangents[i].x);
                    vertices.push_back(ai_mesh->mTangents[i].y);
                    vertices.push_back(ai_mesh->mTangents[i].z);
                    vertices.push_back(1);
                }
                if (attributes[j] == VertexAttributeBit::Color)
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
                if (attributes[j] == VertexAttributeBit::SkinPack)
                {
                    if (mesh->isSkin)
                    {
                        ModelVertexSkin& skin = skin_info_map[i];

                        size_t idx0       = skin.indices[0];
                        size_t idx1       = skin.indices[1];
                        size_t idx2       = skin.indices[2];
                        size_t idx3       = skin.indices[3];
                        size_t pack_index = (idx0 << 24) + (idx1 << 16) + (idx2 << 8) + idx3;

                        uint32_t weight0      = uint32_t(skin.weights[0] * 65535);
                        uint32_t weight1      = uint32_t(skin.weights[1] * 65535);
                        uint32_t weight2      = uint32_t(skin.weights[2] * 65535);
                        uint32_t weight3      = uint32_t(skin.weights[3] * 65535);
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
                if (attributes[j] == VertexAttributeBit::SkinIndex)
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
                if (attributes[j] == VertexAttributeBit::SkinWeight)
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
                if (attributes[j] == VertexAttributeBit::Custom0 || attributes[j] == VertexAttributeBit::Custom1 ||
                    attributes[j] == VertexAttributeBit::Custom2 || attributes[j] == VertexAttributeBit::Custom3)
                {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
            }
        }
    }

    void Model::LoadIndices(std::vector<uint32_t>& indices, const aiMesh* ai_mesh, const aiScene* ai_scene)
    {
        for (size_t i = 0; i < (size_t)ai_mesh->mNumFaces; ++i)
        {
            indices.push_back(ai_mesh->mFaces[i].mIndices[0]);
            indices.push_back(ai_mesh->mFaces[i].mIndices[1]);
            indices.push_back(ai_mesh->mFaces[i].mIndices[2]);
        }
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

    void Model::MergeAllMeshes(const vk::raii::PhysicalDevice& physical_device,
                               const vk::raii::Device&         device,
                               const vk::raii::CommandPool&    command_pool,
                               const vk::raii::Queue&          queue)
    {
        if (meshes.size() < 2)
            return;

        auto new_node          = new ModelNode();
        new_node->local_matrix = glm::mat4(1.0f);

        auto new_mesh = new ModelMesh();
        new_node->meshes.push_back(new_mesh);

        nodes_map.clear();
        nodes_map.insert(std::make_pair(new_node->name, new_node));

        int stride = VertexAttributesToSize(attributes) / sizeof(float);

        for (int node_idx = 0; node_idx < linear_nodes.size(); node_idx++)
        {
            ModelNode* cur_node = linear_nodes[node_idx];

            for (int i = 0; i < cur_node->meshes.size(); i++)
            {
                ModelMesh* cur_node_mesh    = cur_node->meshes[i];
                uint32_t   old_vertex_count = new_mesh->vertex_count;

                for (uint32_t j = 0; j < cur_node_mesh->vertices.size(); j += stride)
                {
                    glm::vec4 point(cur_node_mesh->vertices[j],
                                    cur_node_mesh->vertices[j + 1],
                                    cur_node_mesh->vertices[j + 2],
                                    1.0f);
                    glm::vec4 point_global = cur_node->GetGlobalMatrix() * point;

                    cur_node_mesh->vertices[j]     = point_global.x / point_global.w;
                    cur_node_mesh->vertices[j + 1] = point_global.y / point_global.w;
                    cur_node_mesh->vertices[j + 2] = point_global.z / point_global.w;
                }

                for (uint32_t j = 0; j < cur_node_mesh->indices.size(); j++)
                {
                    cur_node_mesh->indices[j] += old_vertex_count;
                }

                new_mesh->vertices.insert(
                    new_mesh->vertices.end(), cur_node_mesh->vertices.begin(), cur_node_mesh->vertices.end());
                new_mesh->indices.insert(
                    new_mesh->indices.end(), cur_node_mesh->indices.begin(), cur_node_mesh->indices.end());

                new_mesh->vertex_count += cur_node_mesh->vertex_count;
                new_mesh->triangle_count += cur_node_mesh->triangle_count;
                new_mesh->bounding.Merge(cur_node_mesh->bounding);
            }
        }

        new_mesh->vertex_buffer_ptr = std::make_shared<VertexBuffer>(
            physical_device, device, command_pool, queue, new_mesh->vertices);
        new_mesh->index_buffer_ptr = std::make_shared<IndexBuffer>(
            physical_device, device, command_pool, queue, new_mesh->indices);

        delete root_node;
        root_node       = new_node;
        root_node->name = "Merged";

        meshes.clear();
        meshes.push_back(new_mesh);
        linear_nodes.clear();
        linear_nodes.push_back(new_node);

        for (size_t i = 0; i < bones.size(); ++i)
        {
            delete bones[i];
        }
        bones.clear();
    }
} // namespace Meow
