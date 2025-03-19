#pragma once

#include "core/math/bounding_box.h"
#include "function/resource/resource_base.h"
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
    struct Model : public ResourceBase
    {
        using NodesMap = std::unordered_map<std::string, ModelNode*>;
        using BonesMap = std::unordered_map<std::string, ModelBone*>;

        std::filesystem::path   root_path;
        ModelNode*              root_node = nullptr;
        std::vector<ModelNode*> linear_nodes;
        std::vector<ModelMesh*> meshes;

        NodesMap nodes_map;

        std::vector<ModelBone*> bones;
        BonesMap                bones_map;

        std::vector<VertexAttributeBit> attributes;
        std::vector<ModelAnimation>     animations;
        size_t                          animIndex = -1;

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

        template<typename VerticesType, typename IndicesType>
        Model(VerticesType&& vertices, IndicesType&& indices, const std::vector<VertexAttributeBit>& attributes)
        {
            uint32_t stride  = VertexAttributesToSize(attributes);
            auto     mesh    = new ModelMesh();
            this->attributes = attributes;

            mesh->vertices     = std::forward<VerticesType>(vertices);
            mesh->indices      = std::forward<IndicesType>(indices);
            mesh->vertex_count = mesh->vertices.size() / stride * 4;

            mesh->RefreshBuffer();

            mesh->bounding.min = glm::vec3(-1.0f, -1.0f, 0.0f);
            mesh->bounding.max = glm::vec3(1.0f, 1.0f, 0.0f);

            root_node       = new ModelNode();
            root_node->name = "RootNode";
            root_node->meshes.push_back(mesh);
            root_node->local_matrix = glm::mat4(1.0f);
            mesh->link_node         = root_node;

            meshes.push_back(mesh);
        }

        /**
         * @brief Load model from file using assimp.
         *
         * Use aiProcess_PreTransformVertices when importing, so model node doesn't need to save local transform matrix.
         *
         * If you keep local transform matrix of model node, it means you should create uniform buffer for each model
         * node. Then when draw a mesh once you should update buffer data once.
         */
        Model(const std::string& file_path, const std::vector<VertexAttributeBit>& attributes);

        ~Model() override
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

        BoundingBox GetBounding();

        void SetAnimation(size_t index);

        ModelAnimation& GetAnimation(size_t index = -1);

        void GotoAnimation(float time);

    protected:
        ModelNode* LoadNode(const aiNode* node, const aiScene* scene);

        ModelMesh* LoadMesh(const aiMesh* mesh, const aiScene* scene);

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

        void MergeAllMeshes(const vk::raii::PhysicalDevice& physical_device,
                            const vk::raii::Device&         device,
                            const vk::raii::CommandPool&    command_pool,
                            const vk::raii::Queue&          queue);
    };
} // namespace Meow
