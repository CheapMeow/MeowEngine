#pragma once

#include "core/base/bitmask.hpp"
#include "core/base/non_copyable.h"
#include "core/math/bounding_box.h"
#include "model_anim.h"
#include "model_bone.h"
#include "model_mesh.h"
#include "model_node.h"
#include "vertex_attribute.h"

#include <assimp/scene.h>
#include <glm/glm.hpp>

#include <filesystem>

namespace Meow
{
    struct Model : NonCopyable
    {
        typedef std::unordered_map<std::string, ModelNode*> NodesMap;
        typedef std::unordered_map<std::string, ModelBone*> BonesMap;

        ModelNode*              root_node = nullptr;
        std::vector<ModelNode*> linear_nodes;
        std::vector<ModelMesh*> meshes;

        NodesMap nodes_map;

        std::vector<ModelBone*> bones;
        BonesMap                bones_map;

        BitMask<VertexAttributeBit> attributes;
        std::vector<ModelAnimation> animations;
        size_t                      animIndex = -1;

        bool loadSkin = false;

        Model(std::nullptr_t) {};

        Model(Model&& rhs) noexcept
        {
            std::swap(root_node, rhs.root_node);
            std::swap(linear_nodes, rhs.linear_nodes);
            std::swap(meshes, rhs.meshes);
            std::swap(nodes_map, rhs.nodes_map);
            std::swap(bones, rhs.bones);
            std::swap(bones_map, rhs.bones_map);
            std::swap(attributes, rhs.attributes);
            std::swap(animations, rhs.animations);
            animIndex = rhs.animIndex;
            loadSkin  = rhs.loadSkin;
        }

        Model& operator=(Model&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::swap(root_node, rhs.root_node);
                std::swap(linear_nodes, rhs.linear_nodes);
                std::swap(meshes, rhs.meshes);
                std::swap(nodes_map, rhs.nodes_map);
                std::swap(bones, rhs.bones);
                std::swap(bones_map, rhs.bones_map);
                std::swap(attributes, rhs.attributes);
                std::swap(animations, rhs.animations);
                animIndex = rhs.animIndex;
                loadSkin  = rhs.loadSkin;
            }
            return *this;
        }

        Model(vk::raii::PhysicalDevice const& physical_device,
              vk::raii::Device const&         device,
              vk::raii::CommandPool const&    command_pool,
              vk::raii::Queue const&          queue,
              std::vector<float>&             vertices,
              std::vector<uint32_t>&          indices,
              BitMask<VertexAttributeBit>     attributes);

        /**
         * @brief Load model from file using assimp.
         *
         * Use aiProcess_PreTransformVertices when importing, so model node doesn't need to save local transform matrix.
         *
         * If you keep local transform matrix of model node, it means you should create uniform buffer for each model
         * node. Then when draw a mesh once you should update buffer data once.
         */
        Model(vk::raii::PhysicalDevice const& physical_device,
              vk::raii::Device const&         device,
              vk::raii::CommandPool const&    command_pool,
              vk::raii::Queue const&          queue,
              const std::string&              file_path,
              BitMask<VertexAttributeBit>     attributes);

        ~Model()
        {
            delete root_node;
            root_node = nullptr;

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

        ModelAnimation& GetAnimation(size_t index = -1);

        void GotoAnimation(float time);

    protected:
        ModelNode* LoadNode(vk::raii::PhysicalDevice const& physical_device,
                            vk::raii::Device const&         device,
                            vk::raii::CommandPool const&    command_pool,
                            vk::raii::Queue const&          queue,
                            const aiNode*                   node,
                            const aiScene*                  scene);

        ModelMesh* LoadMesh(vk::raii::PhysicalDevice const& physical_device,
                            vk::raii::Device const&         device,
                            vk::raii::CommandPool const&    command_pool,
                            vk::raii::Queue const&          queue,
                            const aiMesh*                   mesh,
                            const aiScene*                  scene);

        void LoadBones(const aiScene* aiScene);

        void LoadSkin(std::unordered_map<size_t, ModelVertexSkin>& skin_info_map,
                      ModelMesh*                                   mesh,
                      const aiMesh*                                aiMesh,
                      const aiScene*                               aiScene);

        void LoadVertexDatas(std::unordered_map<size_t, ModelVertexSkin>& skin_info_map,
                             std::vector<float>&                          vertices,
                             glm::vec3&                                   mmax,
                             glm::vec3&                                   mmin,
                             ModelMesh*                                   mesh,
                             const aiMesh*                                ai_mesh,
                             const aiScene*                               ai_scene);

        void LoadIndices(std::vector<uint32_t>& indices, const aiMesh* ai_mesh, const aiScene* ai_scene);

        void LoadAnim(const aiScene* ai_scene);

        void MergeAllMeshes(vk::raii::PhysicalDevice const& physical_device,
                            vk::raii::Device const&         device,
                            vk::raii::CommandPool const&    command_pool,
                            vk::raii::Queue const&          queue);
    };
} // namespace Meow
