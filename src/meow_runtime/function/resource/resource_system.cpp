#include "resource_system.h"

#include "pch.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    ResourceSystem::ResourceSystem() {}

    ResourceSystem::~ResourceSystem() {}

    void ResourceSystem::Start()
    {
        // TODO: Image size should be analysised?
        // std::shared_ptr<ImageData> diffuse_texture = LoadTexture("builtin/models/backpack/diffuse.jpg", {4096,
        // 4096}); m_materials["Default Material"] =
        //     std::make_shared<Material>(g_runtime_context.render_system->CreateMaterial(
        //         "builtin/shaders/textured_mesh_without_vertex_color.vert.spv",
        //         "builtin/shaders/textured_mesh_without_vertex_color.frag.spv", diffuse_texture));
    }

    void ResourceSystem::Tick(float dt) {}

    std::tuple<bool, UUID> ResourceSystem::LoadTexture(const std::string& file_path)
    {
        FUNCTION_TIMER();

        if (m_textures_path2id.find(file_path) != m_textures_path2id.end())
        {
            UUID uuid = m_textures_path2id[file_path];
            if (m_textures_id2data.find(uuid) != m_textures_id2data.end())
            {
                return {true, uuid};
            }
        }

        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        std::shared_ptr<ImageData> texture_ptr = ImageData::CreateTexture(
            physical_device, logical_device, onetime_submit_command_pool, graphics_queue, file_path);

        if (texture_ptr)
        {
            m_textures_path2id[file_path]         = texture_ptr->uuid;
            m_textures_id2data[texture_ptr->uuid] = texture_ptr;
            return {true, texture_ptr->uuid};
        }
        else
        {
            return {false, UUID(0)};
        }
    }

    std::shared_ptr<ImageData> ResourceSystem::GetTexture(const UUID& uuid)
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

    std::tuple<bool, UUID> ResourceSystem::LoadModel(std::vector<float>&&            vertices,
                                                     std::vector<uint32_t>&&         indices,
                                                     std::vector<VertexAttributeBit> attributes)
    {
        FUNCTION_TIMER();

        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        auto model_ptr = std::make_shared<Model>(physical_device,
                                                 logical_device,
                                                 onetime_submit_command_pool,
                                                 graphics_queue,
                                                 std::move(vertices),
                                                 std::move(indices),
                                                 attributes);

        if (model_ptr)
        {
            m_models_id2data[model_ptr->uuid] = model_ptr;
            return {true, model_ptr->uuid};
        }
        else
        {
            return {false, UUID(0)};
        }
    }

    std::tuple<bool, UUID> ResourceSystem::LoadModel(const std::string&              file_path,
                                                     std::vector<VertexAttributeBit> attributes)
    {
        FUNCTION_TIMER();

        if (m_models_path2id.find(file_path) != m_models_path2id.end())
        {
            UUID uuid = m_models_path2id[file_path];
            if (m_models_id2data.find(uuid) != m_models_id2data.end())
            {
                return {true, uuid};
            }
        }

        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        auto model_ptr = std::make_shared<Model>(
            physical_device, logical_device, onetime_submit_command_pool, graphics_queue, file_path, attributes);

        if (model_ptr)
        {
            m_models_path2id[file_path]       = model_ptr->uuid;
            m_models_id2data[model_ptr->uuid] = model_ptr;
            return {true, model_ptr->uuid};
        }
        else
        {
            return {false, UUID(0)};
        }
    }

    std::shared_ptr<Model> ResourceSystem::GetModel(const UUID& uuid)
    {
        FUNCTION_TIMER();

        if (m_models_id2data.find(uuid) == m_models_id2data.end())
            return nullptr;

        return m_models_id2data[uuid];
    }
} // namespace Meow
