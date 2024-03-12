#pragma once

#include "core/math/bounding_box.h"
#include "function/systems/render/structs/material.h"

#include "model_anim.h"
#include "model_bone.h"
#include "model_mesh.h"
#include "model_node.h"

#include <assimp/scene.h>
#include <glm/glm.hpp>

#include <filesystem>

namespace Meow
{
    struct Bone
    {
        std::string name;
        size_t      index  = -1;
        size_t      parent = -1;
        Matrix4x4   inverseBindPose;
        Matrix4x4   finalTransform;
    };

    struct VertexSkin
    {
        size_t used = 0;
        size_t indices[4];
        float  weights[4];
    };

    struct Model
    {
        typedef std::unordered_map<std::string, ModelNode*> NodesMap;
        typedef std::unordered_map<std::string, ModelBone*> BonesMap;

        ModelNode*              root_node;
        std::vector<ModelNode*> linear_nodes;
        std::vector<ModelMesh*> meshes;

        NodesMap nodesMap;

        std::vector<ModelBone*> bones;
        BonesMap                bones_map;

        std::vector<VertexAttribute> attributes;
        std::vector<ModelAnimation>  animations;
        size_t                       animIndex = -1;

        bool loadSkin = false;

        Model(vk::raii::PhysicalDevice const& physical_device,
              vk::raii::Device const&         device,
              vk::raii::CommandPool const&    command_pool,
              vk::raii::Queue const&          queue,
              const std::string&              file_path,
              std::vector<VertexAttribute>    attributes,
              vk::IndexType                   index_type);

        ~DVKModel()
        {
            delete root_node;
            root_node = nullptr;
            device    = nullptr;

            meshes.clear();
            linear_nodes.clear();

            for (size_t i = 0; i < bones.size(); ++i)
            {
                delete bones[i];
            }
            bones.clear();
        }

        void Update(float time, float delta);

        void SetAnimation(size_t index);

        DVKAnimation& GetAnimation(size_t index = -1);

        void GotoAnimation(float time);

        VkVertexInputBindingDescription GetInputBinding();

        std::vector<VkVertexInputAttributeDescription> GetInputAttributes();

    protected:
        DVKNode* LoadNode(const aiNode* node, const aiScene* scene);

        DVKMesh* LoadMesh(const aiMesh* mesh, const aiScene* scene);

        void LoadBones(const aiScene* aiScene);

        void LoadSkin(std::unordered_map<size_t, DVKVertexSkin>& skinInfoMap,
                      DVKMesh*                                   mesh,
                      const aiMesh*                              aiMesh,
                      const aiScene*                             aiScene);

        void LoadVertexDatas(std::unordered_map<size_t, DVKVertexSkin>& skinInfoMap,
                             std::vector<float>&                        vertices,
                             Vector3&                                   mmax,
                             Vector3&                                   mmin,
                             DVKMesh*                                   mesh,
                             const aiMesh*                              ai_mesh,
                             const aiScene*                             ai_scene);

        void LoadIndices(std::vector<size_t>& indices, const aiMesh* ai_mesh, const aiScene* ai_scene);

        void LoadPrimitives(std::vector<float>&  vertices,
                            std::vector<size_t>& indices,
                            DVKMesh*             mesh,
                            const aiMesh*        ai_mesh,
                            const aiScene*       ai_scene);

        void LoadAnim(const aiScene* ai_scene);
    };
} // namespace Meow
