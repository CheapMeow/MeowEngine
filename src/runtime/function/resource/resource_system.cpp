#include "resource_system.h"

#include "pch.h"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    ResourceSystem::ResourceSystem() {}

    ResourceSystem::~ResourceSystem() {}

    void ResourceSystem::Start()
    {
        // TODO: Image size should be analysised?
        // std::shared_ptr<ImageData> diffuse_texture = LoadTexture("builtin/models/backpack/diffuse.jpg", {4096,
        // 4096}); m_materials["Default Material"] =
        //     std::make_shared<Material>(g_runtime_global_context.render_system->CreateMaterial(
        //         "builtin/shaders/textured_mesh_without_vertex_color.vert.spv",
        //         "builtin/shaders/textured_mesh_without_vertex_color.frag.spv", diffuse_texture));
    }

    void ResourceSystem::Tick(float dt) {}

    bool ResourceSystem::LoadTexture(const std::string& file_path, UUIDv4::UUID& uuid)
    {
        FUNCTION_TIMER();

        if (m_textures_tag2id.find(file_path) != m_textures_tag2id.end())
        {
            uuid = m_textures_tag2id[file_path];
            if (m_textures_id2data.find(uuid) != m_textures_id2data.end())
            {
                return true;
            }
        }

        std::shared_ptr<ImageData> texture_ptr = g_runtime_global_context.render_system->CreateTexture(file_path);

        if (texture_ptr)
        {
            uuid                         = m_uuid_generator.getUUID();
            m_textures_tag2id[file_path] = uuid;
            m_textures_id2data[uuid]     = texture_ptr;
            return true;
        }
        else
        {
            return false;
        }
    }

    std::shared_ptr<ImageData> ResourceSystem::GetTexture(const UUIDv4::UUID& uuid)
    {
        FUNCTION_TIMER();

        if (m_textures_id2data.find(uuid) == m_textures_id2data.end())
            return nullptr;

        return m_textures_id2data[uuid];
    }

    // bool ResourceSystem::LoadMaterial(const std::string& file_path)
    // {
    //     // Haven't implemented.
    //     return true;
    // }

    // std::shared_ptr<Material> ResourceSystem::GetMaterial(const std::string& file_path)
    // {
    //     if (m_materials.find(file_path) == m_materials.end())
    //         return nullptr;

    //     return m_materials[file_path];
    // }

    bool
    ResourceSystem::LoadModel(const std::string& file_path, BitMask<VertexAttributeBit> attributes, UUIDv4::UUID& uuid)
    {
        FUNCTION_TIMER();

        if (m_models_tag2id.find(file_path) != m_models_tag2id.end())
        {
            uuid = m_models_tag2id[file_path];
            if (m_models_id2data.find(uuid) != m_models_id2data.end())
            {
                return true;
            }
        }

        std::shared_ptr<Model> model_ptr = g_runtime_global_context.render_system->CreateModel(file_path, attributes);

        if (model_ptr)
        {
            uuid                       = m_uuid_generator.getUUID();
            m_models_tag2id[file_path] = uuid;
            m_models_id2data[uuid]     = model_ptr;
            return true;
        }
        else
        {
            return false;
        }
    }

    std::shared_ptr<Model> ResourceSystem::GetModel(const UUIDv4::UUID& uuid)
    {
        FUNCTION_TIMER();

        if (m_models_id2data.find(uuid) == m_models_id2data.end())
            return nullptr;

        return m_models_id2data[uuid];
    }
} // namespace Meow
