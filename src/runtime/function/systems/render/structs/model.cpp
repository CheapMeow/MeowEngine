#include "model.h"

#include "core/math/assimp_glm_helper.h"
#include "function/global/runtime_global_context.h"
#include "vertex_attribute.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

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
        LoadNode(scene->mRootNode, scene);
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

    void FillMatrixWithAiMatrix(Matrix4x4& matrix, const aiMatrix4x4& ai_matrix)
    {
        matrix.m[0][0] = ai_matrix.a1;
        matrix.m[0][1] = ai_matrix.a2;
        matrix.m[0][2] = ai_matrix.a3;
        matrix.m[0][3] = ai_matrix.a4;
        matrix.m[1][0] = ai_matrix.b1;
        matrix.m[1][1] = ai_matrix.b2;
        matrix.m[1][2] = ai_matrix.b3;
        matrix.m[1][3] = ai_matrix.b4;
        matrix.m[2][0] = ai_matrix.c1;
        matrix.m[2][1] = ai_matrix.c2;
        matrix.m[2][2] = ai_matrix.c3;
        matrix.m[2][3] = ai_matrix.c4;
        matrix.m[3][0] = ai_matrix.d1;
        matrix.m[3][1] = ai_matrix.d2;
        matrix.m[3][2] = ai_matrix.d3;
        matrix.m[3][3] = ai_matrix.d4;
        matrix.SetTransposed();
    }

    void Model::LoadBones(const aiScene* ai_scene)
    {
        std::unordered_map<std::string, size_t> boneIndexMap;
        for (size_t i = 0; i < (size_t)ai_scene->mNumMeshes; ++i)
        {
            aiMesh* ai_mesh = ai_scene->mMeshes[i];
            for (size_t j = 0; j < (size_t)ai_mesh->mNumBones; ++j)
            {
                aiBone*     ai_bone = ai_mesh->mBones[j];
                std::string name    = ai_bone->mName.C_Str();

                auto it = boneIndexMap.find(name);
                if (it == boneIndexMap.end())
                {
                    // new bone
                    size_t index = (size_t)bones.size();
                    Bone*  bone  = new Bone();
                    bone->index  = index;
                    bone->parent = -1;
                    bone->name   = name;
                    FillMatrixWithAiMatrix(bone->inverseBindPose, ai_bone->mOffsetMatrix);
                    // 记录Bone信息
                    bones.push_back(bone);
                    bones_map.insert(std::make_pair(name, bone));
                    // cache
                    boneIndexMap.insert(std::make_pair(name, index));
                }
            }
        }
    }

    void Model::LoadSkin(std::unordered_map<size_t, VertexSkin>& skinInfoMap,
                         Mesh*                                   mesh,
                         const aiMesh*                           ai_mesh,
                         const aiScene*                          ai_scene)
    {
        std::unordered_map<size_t, size_t> boneIndexMap;

        for (size_t i = 0; i < (size_t)ai_mesh->mNumBones; ++i)
        {
            aiBone*     boneInfo = ai_mesh->mBones[i];
            std::string boneName(boneInfo->mName.C_Str());
            size_t      boneIndex = bones_map[boneName]->index;

            // bone在mesh中的索引
            size_t meshBoneIndex = 0;
            auto   it            = boneIndexMap.find(boneIndex);
            if (it == boneIndexMap.end())
            {
                meshBoneIndex = (size_t)mesh->bones.size();
                mesh->bones.push_back(boneIndex);
                boneIndexMap.insert(std::make_pair(boneIndex, meshBoneIndex));
            }
            else
            {
                meshBoneIndex = it->second;
            }

            // 收集被Bone影响的顶点信息
            for (size_t j = 0; j < boneInfo->mNumWeights; ++j)
            {
                size_t vertexID = boneInfo->mWeights[j].mVertexId;
                float  weight   = boneInfo->mWeights[j].mWeight;
                // 顶点->Bone
                if (skinInfoMap.find(vertexID) == skinInfoMap.end())
                {
                    skinInfoMap.insert(std::make_pair(vertexID, VertexSkin()));
                }
                VertexSkin* info          = &(skinInfoMap[vertexID]);
                info->indices[info->used] = meshBoneIndex;
                info->weights[info->used] = weight;
                info->used += 1;
                // 只允许最多四个骨骼影响顶点
                if (info->used >= 4)
                {
                    break;
                }
            }
        }

        // 再次处理一遍skinInfoMap，把未使用的补齐
        for (auto it = skinInfoMap.begin(); it != skinInfoMap.end(); ++it)
        {
            VertexSkin& info = it->second;
            for (size_t i = info.used; i < 4; ++i)
            {
                info.indices[i] = 0;
                info.weights[i] = 0.0f;
            }
        }

        mesh->isSkin = true;
    }

    void Model::LoadVertexDatas(std::unordered_map<size_t, VertexSkin>& skinInfoMap,
                                std::vector<float>&                     vertices,
                                Vector3&                                mmax,
                                Vector3&                                mmin,
                                Mesh*                                   mesh,
                                const aiMesh*                           ai_mesh,
                                const aiScene*                          ai_scene)
    {
        Vector3 defaultColor(MMath::RandRange(0.0f, 1.0f), MMath::RandRange(0.0f, 1.0f), MMath::RandRange(0.0f, 1.0f));

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

                    mmin.x = MMath::Min(v0, mmin.x);
                    mmin.y = MMath::Min(v1, mmin.y);
                    mmin.z = MMath::Min(v2, mmin.z);
                    mmax.x = MMath::Max(v0, mmax.x);
                    mmax.y = MMath::Max(v1, mmax.y);
                    mmax.z = MMath::Max(v2, mmax.z);
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
                        VertexSkin& skin = skinInfoMap[i];

                        size_t idx0      = skin.indices[0];
                        size_t idx1      = skin.indices[1];
                        size_t idx2      = skin.indices[2];
                        size_t idx3      = skin.indices[3];
                        size_t packIndex = (idx0 << 24) + (idx1 << 16) + (idx2 << 8) + idx3;

                        uint16 weight0     = uint16(skin.weights[0] * 65535);
                        uint16 weight1     = uint16(skin.weights[1] * 65535);
                        uint16 weight2     = uint16(skin.weights[2] * 65535);
                        uint16 weight3     = uint16(skin.weights[3] * 65535);
                        size_t packWeight0 = (weight0 << 16) + weight1;
                        size_t packWeight1 = (weight2 << 16) + weight3;

                        vertices.push_back((float)packIndex);
                        vertices.push_back((float)packWeight0);
                        vertices.push_back((float)packWeight1);
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
                        VertexSkin& skin = skinInfoMap[i];
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
                        VertexSkin& skin = skinInfoMap[i];
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

    void Model::LoadModelPrimitives(std::vector<float>&  vertices,
                                    std::vector<size_t>& indices,
                                    Mesh*                mesh,
                                    const aiMesh*        aiMesh,
                                    const aiScene*       ai_scene)
    {
        size_t stride = (size_t)vertices.size() / aiMesh->mNumVertices;

        if (indices.size() > 65535)
        {
            std::unordered_map<size_t, size_t> indicesMap;
            ModelPrimitive*                    primitive = nullptr;

            for (size_t i = 0; i < indices.size(); ++i)
            {
                size_t idx = indices[i];
                if (primitive == nullptr)
                {
                    primitive = new ModelPrimitive();
                    indicesMap.clear();
                    mesh->primitives.push_back(primitive);
                }

                size_t newIdx = 0;
                auto   it     = indicesMap.find(idx);
                if (it == indicesMap.end())
                {
                    size_t start = idx * stride;
                    newIdx       = (size_t)primitive->vertices.size() / stride;
                    primitive->vertices.insert(
                        primitive->vertices.end(), vertices.begin() + start, vertices.begin() + start + stride);
                    indicesMap.insert(std::make_pair(idx, newIdx));
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

            if (cmdBuffer)
            {
                for (size_t i = 0; i < mesh->primitives.size(); ++i)
                {
                    primitive               = mesh->primitives[i];
                    primitive->vertexBuffer = VertexBuffer::Create(device, cmdBuffer, primitive->vertices, attributes);
                    primitive->indexBuffer  = IndexBuffer::Create(device, cmdBuffer, primitive->indices);
                }
            }
        }
        else
        {
            ModelPrimitive* primitive = new ModelPrimitive();
            primitive->vertices       = vertices;
            for (uint16 i = 0; i < indices.size(); ++i)
            {
                primitive->indices.push_back(indices[i]);
            }
            mesh->primitives.push_back(primitive);

            if (cmdBuffer)
            {
                primitive->vertexBuffer = VertexBuffer::Create(device, cmdBuffer, primitive->vertices, attributes);
                primitive->indexBuffer  = IndexBuffer::Create(device, cmdBuffer, primitive->indices);
            }
        }

        for (size_t i = 0; i < mesh->primitives.size(); ++i)
        {
            ModelPrimitive* primitive = mesh->primitives[i];
            primitive->vertexCount    = (size_t)primitive->vertices.size() / stride;
            primitive->triangleNum    = (size_t)primitive->indices.size() / 3;

            mesh->vertexCount += primitive->vertexCount;
            mesh->triangleCount += primitive->triangleNum;
        }
    }

    Mesh* Model::LoadMesh(const aiMesh* ai_mesh, const aiScene* ai_scene)
    {
        Mesh* mesh = new Mesh();

        // load material
        aiMaterial* material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
        if (material)
        {
            FillMaterialTextures(material, mesh->material);
        }

        // load bones
        std::unordered_map<size_t, VertexSkin> skinInfoMap;
        if (ai_mesh->mNumBones > 0 && loadSkin)
        {
            LoadSkin(skinInfoMap, mesh, ai_mesh, ai_scene);
        }

        // load vertex data
        std::vector<float> vertices;
        Vector3            mmin(MAX_FLT, MAX_FLT, MAX_FLT);
        Vector3            mmax(-MAX_FLT, -MAX_FLT, -MAX_FLT);
        LoadVertexDatas(skinInfoMap, vertices, mmax, mmin, mesh, ai_mesh, ai_scene);

        // load indices
        std::vector<size_t> indices;
        LoadIndices(indices, ai_mesh, ai_scene);

        // load primitives
        LoadModelPrimitives(vertices, indices, mesh, ai_mesh, ai_scene);

        mesh->bounding.min = mmin;
        mesh->bounding.max = mmax;
        mesh->bounding.UpdateCorners();

        return mesh;
    }

    Node* Model::LoadNode(const aiNode* aiNode, const aiScene* ai_scene)
    {
        Node* vkNode = new Node();
        vkNode->name = aiNode->mName.C_Str();

        if (root_node == nullptr)
        {
            root_node = vkNode;
        }

        // local matrix
        FillMatrixWithAiMatrix(vkNode->localMatrix, aiNode->mTransformation);

        // mesh
        if (aiNode->mNumMeshes > 0)
        {
            for (size_t i = 0; i < aiNode->mNumMeshes; ++i)
            {
                Mesh* vkMesh     = LoadMesh(ai_scene->mMeshes[aiNode->mMeshes[i]], ai_scene);
                vkMesh->linkNode = vkNode;
                vkNode->meshes.push_back(vkMesh);
                meshes.push_back(vkMesh);
            }
        }

        // nodes map
        nodesMap.insert(std::make_pair(vkNode->name, vkNode));
        linear_nodes.push_back(vkNode);

        // bones parent
        size_t boneParentIndex = -1;
        {
            auto it = bones_map.find(vkNode->name);
            if (it != bones_map.end())
            {
                boneParentIndex = it->second->index;
            }
        }

        // children node
        for (size_t i = 0; i < (size_t)aiNode->mNumChildren; ++i)
        {
            Node* childNode   = LoadNode(aiNode->mChildren[i], ai_scene);
            childNode->parent = vkNode;
            vkNode->children.push_back(childNode);

            // bones relationship
            {
                auto it = bones_map.find(childNode->name);
                if (it != bones_map.end())
                {
                    it->second->parent = boneParentIndex;
                }
            }
        }

        return vkNode;
    }

    void Model::LoadAnim(const aiScene* ai_scene)
    {
        for (size_t i = 0; i < (size_t)ai_scene->mNumAnimations; ++i)
        {
            aiAnimation* aianimation = ai_scene->mAnimations[i];
            float        timeTick    = aianimation->mTicksPerSecond != 0 ? (float)aianimation->mTicksPerSecond : 25.0f;

            animations.push_back(Animation());
            Animation& dvkAnimation = animations.back();

            for (size_t j = 0; j < (size_t)aianimation->mNumChannels; ++j)
            {
                aiNodeAnim* nodeAnim = aianimation->mChannels[j];
                std::string nodeName = nodeAnim->mNodeName.C_Str();

                dvkAnimation.clips.insert(std::make_pair(nodeName, AnimationClip()));

                AnimationClip& animClip = dvkAnimation.clips[nodeName];
                animClip.nodeName       = nodeName;
                animClip.duration       = 0.0f;

                // position
                for (size_t index = 0; index < (size_t)nodeAnim->mNumPositionKeys; ++index)
                {
                    aiVectorKey& aikey = nodeAnim->mPositionKeys[index];
                    animClip.positions.keys.push_back((float)aikey.mTime / timeTick);
                    animClip.positions.values.push_back(Vector3(aikey.mValue.x, aikey.mValue.y, aikey.mValue.z));
                    animClip.duration = MMath::Max((float)aikey.mTime / timeTick, animClip.duration);
                }

                // scale
                for (size_t index = 0; index < (size_t)nodeAnim->mNumScalingKeys; ++index)
                {
                    aiVectorKey& aikey = nodeAnim->mScalingKeys[index];
                    animClip.scales.keys.push_back((float)aikey.mTime / timeTick);
                    animClip.scales.values.push_back(Vector3(aikey.mValue.x, aikey.mValue.y, aikey.mValue.z));
                    animClip.duration = MMath::Max((float)aikey.mTime / timeTick, animClip.duration);
                }

                // rotation
                for (size_t index = 0; index < (size_t)nodeAnim->mNumRotationKeys; ++index)
                {
                    aiQuatKey& aikey = nodeAnim->mRotationKeys[index];
                    animClip.rotations.keys.push_back((float)aikey.mTime / timeTick);
                    animClip.rotations.values.push_back(
                        Quat(aikey.mValue.x, aikey.mValue.y, aikey.mValue.z, aikey.mValue.w));
                    animClip.duration = MMath::Max((float)aikey.mTime / timeTick, animClip.duration);
                }

                dvkAnimation.duration = MMath::Max(animClip.duration, dvkAnimation.duration);
            }
        }
    }

    void Model::GotoAnimation(float time)
    {
        if (animIndex == -1)
        {
            return;
        }

        Animation& animation = animations[animIndex];
        animation.time       = MMath::Clamp(time, 0.0f, animation.duration);

        // update nodes animation
        for (auto it = animation.clips.begin(); it != animation.clips.end(); ++it)
        {
            vk_demo::AnimationClip& clip = it->second;
            vk_demo::Node*          node = nodesMap[clip.nodeName];

            float alpha = 0.0f;

            // rotation
            Quat prevRot(0, 0, 0, 1);
            Quat nextRot(0, 0, 0, 1);
            clip.rotations.GetValue(animation.time, prevRot, nextRot, alpha);
            Quat retRot = MMath::Lerp(prevRot, nextRot, alpha);

            // position
            Vector3 prevPos(0, 0, 0);
            Vector3 nextPos(0, 0, 0);
            clip.positions.GetValue(animation.time, prevPos, nextPos, alpha);
            Vector3 retPos = MMath::Lerp(prevPos, nextPos, alpha);

            // scale
            Vector3 prevScale(1, 1, 1);
            Vector3 nextScale(1, 1, 1);
            clip.scales.GetValue(animation.time, prevScale, nextScale, alpha);
            Vector3 retScale = MMath::Lerp(prevScale, nextScale, alpha);

            node->localMatrix.SetIdentity();
            node->localMatrix.AppendScale(retScale);
            node->localMatrix.Append(retRot.ToMatrix());
            node->localMatrix.AppendTranslation(retPos);
        }

        // update bones
        for (size_t i = 0; i < bones.size(); ++i)
        {
            Bone* bone = bones[i];
            Node* node = nodesMap[bone->name];
            // 注意行列矩阵的区别
            bone->finalTransform = bone->inverseBindPose;
            bone->finalTransform.Append(node->GetGlobalMatrix());
        }
    }

    void Model::Update(float time, float delta)
    {
        if (animIndex == -1)
        {
            return;
        }

        Animation& animation = animations[animIndex];
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

    Animation& Model::GetAnimation(size_t index)
    {
        if (index == -1)
        {
            index = animIndex;
        }
        return animations[index];
    }

    VkVertexInputBindingDescription Model::GetInputBinding()
    {
        size_t stride = 0;
        for (size_t i = 0; i < attributes.size(); ++i)
        {
            stride += VertexAttributeToSize(attributes[i]);
        }

        VkVertexInputBindingDescription vertexInputBinding = {};
        vertexInputBinding.binding                         = 0;
        vertexInputBinding.stride                          = stride;
        vertexInputBinding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

        return vertexInputBinding;
    }

    std::vector<VkVertexInputAttributeDescription> Model::GetInputAttributes()
    {
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributs;
        size_t                                         offset = 0;

        for (size_t i = 0; i < attributes.size(); ++i)
        {
            VkVertexInputAttributeDescription inputAttribute = {};
            inputAttribute.binding                           = 0;
            inputAttribute.location                          = i;
            inputAttribute.format                            = VertexAttributeToVkFormat(attributes[i]);
            inputAttribute.offset                            = offset;
            offset += VertexAttributeToSize(attributes[i]);
            vertexInputAttributs.push_back(inputAttribute);
        }

        return vertexInputAttributs;
    }

} // namespace Meow
