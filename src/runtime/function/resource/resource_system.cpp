#include "resource_system.h"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    ResourceSystem::ResourceSystem() {}

    ResourceSystem::~ResourceSystem() {}

    void ResourceSystem::Start()
    {
        // TODO: Image size should be analysised?
        // std::shared_ptr<TextureData> diffuse_texture = LoadTexture("builtin/models/backpack/diffuse.jpg", {4096,
        // 4096}); m_materials["Default Material"] =
        //     std::make_shared<Material>(g_runtime_global_context.render_system->CreateMaterial(
        //         "builtin/shaders/textured_mesh_without_vertex_color.vert.spv",
        //         "builtin/shaders/textured_mesh_without_vertex_color.frag.spv", diffuse_texture));
    }

    void ResourceSystem::Update(float frame_time) {}

    std::shared_ptr<TextureData> ResourceSystem::LoadTexture(const std::string& filepath)
    {
        std::shared_ptr<TextureData> texture_ptr = g_runtime_global_context.render_system->CreateImageFromFile(filepath);
        g_runtime_global_context.render_system->EnQueueRenderCommand(
            [texture_ptr](vk::raii::CommandBuffer const& command_buffer) {
                texture_ptr->TransitLayout(command_buffer);
            });

        return m_textures[filepath];
    }

    std::shared_ptr<TextureData> ResourceSystem::GetTexture(const std::string& filepath)
    {
        if (m_textures.find(filepath) == m_textures.end())
            return nullptr;

        return m_textures[filepath];
    }

    // bool ResourceSystem::LoadMaterial(const std::string& filepath)
    // {
    //     // Haven't implemented.
    //     return true;
    // }

    // std::shared_ptr<Material> ResourceSystem::GetMaterial(const std::string& filepath)
    // {
    //     if (m_materials.find(filepath) == m_materials.end())
    //         return nullptr;

    //     return m_materials[filepath];
    // }
} // namespace Meow
