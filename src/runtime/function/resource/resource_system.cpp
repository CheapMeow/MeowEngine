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

    std::shared_ptr<ImageData> ResourceSystem::LoadTexture(const std::string& file_path)
    {
        FUNCTION_TIMER();

        std::shared_ptr<ImageData> texture_ptr =
            g_runtime_global_context.render_system->CreateTextureFromFile(file_path);

        m_textures[file_path] = texture_ptr;

        return texture_ptr;
    }

    std::shared_ptr<ImageData> ResourceSystem::GetTexture(const std::string& file_path)
    {
        FUNCTION_TIMER();

        if (m_textures.find(file_path) == m_textures.end())
            return nullptr;

        return m_textures[file_path];
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

    std::shared_ptr<Model> ResourceSystem::LoadModel(const std::string&          file_path,
                                                     BitMask<VertexAttributeBit> attributes)
    {
        FUNCTION_TIMER();

        std::shared_ptr<Model> model_ptr =
            g_runtime_global_context.render_system->CreateModelFromFile(file_path, attributes);

        m_models[file_path] = model_ptr;

        return model_ptr;
    }

    std::shared_ptr<Model> ResourceSystem::GetModel(const std::string& file_path)
    {
        FUNCTION_TIMER();

        if (m_models.find(file_path) == m_models.end())
            return nullptr;

        return m_models[file_path];
    }
} // namespace Meow
