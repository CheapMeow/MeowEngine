#pragma once

#include "core/math/bounding_box.h"
#include "function/systems/render/structs/material.h"
#include "function/systems/render/structs/mesh.h"

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <unordered_map>

namespace Meow
{
    class Model
    {
    public:
        Model(const std::string&              file_name,
              vk::raii::PhysicalDevice const& physical_device,
              vk::raii::Device const&         device,
              std::vector<VertexAttribute>    attributes)
            : m_attributes(attributes)
        {
            LoadFromFile(physical_device, device, file_name);
        }

        VkVertexInputBindingDescription GetInputBinding();

        std::vector<VkVertexInputAttributeDescription> GetInputAttributes();

        BoundingBox GetBounding() { return m_bounding; }

        void Draw(const vk::raii::CommandBuffer& cmd_buffer);

    protected:
        std::vector<uint32_t> LoadMaterialTextures(const aiMaterial* mat, const aiTextureType type);

        void LoadFromFile(vk::raii::PhysicalDevice const& physical_device,
                          vk::raii::Device const&         device,
                          const std::string&              file_name);

        void LoadNode(vk::raii::PhysicalDevice const& physical_device,
                      vk::raii::Device const&         device,
                      const aiNode*                   node,
                      const aiScene*                  scene);

        void LoadMesh(vk::raii::PhysicalDevice const& physical_device,
                      vk::raii::Device const&         device,
                      const aiMesh*                   mesh,
                      const aiScene*                  scene);

    public:
        std::string                                                      m_directory;
        std::vector<std::string>                                         m_texture_paths;
        std::vector<MaterialInfo>                                        m_material_infos;
        std::vector<VertexAttribute>                                     m_attributes;
        vk::IndexType                                                    m_index_type = vk::IndexType::eUint16;
        std::unordered_map<uint32_t, std::vector<std::shared_ptr<Mesh>>> m_meshes_per_material_map;
        BoundingBox                                                      m_bounding;
    };
} // namespace Meow
