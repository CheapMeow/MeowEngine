#pragma once

#include "core/reflect/reflect_pointer.hpp"
#include "function/object/game_object.h"
#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"
#include "render_pass.h"

#include <functional>
#include <glm/glm.hpp>
#include <stack>
#include <string>
#include <unordered_map>

namespace Meow
{
    class ImguiPass : public RenderPass
    {
    public:
        ImguiPass(std::nullptr_t);

        ImguiPass(ImguiPass&& rhs) noexcept
            : RenderPass(nullptr)
        {
            std::swap(render_pass, rhs.render_pass);
            std::swap(framebuffers, rhs.framebuffers);
            std::swap(clear_values, rhs.clear_values);
            std::swap(input_vertex_attributes, rhs.input_vertex_attributes);
            m_depth_format = rhs.m_depth_format;
            m_sample_count = rhs.m_sample_count;
            std::swap(m_depth_attachment, rhs.m_depth_attachment);
            std::swap(query_pool, rhs.query_pool);
            enable_query = rhs.enable_query;
        }

        ImguiPass& operator=(ImguiPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::swap(render_pass, rhs.render_pass);
                std::swap(framebuffers, rhs.framebuffers);
                std::swap(clear_values, rhs.clear_values);
                std::swap(input_vertex_attributes, rhs.input_vertex_attributes);
                m_depth_format = rhs.m_depth_format;
                m_sample_count = rhs.m_sample_count;
                std::swap(m_depth_attachment, rhs.m_depth_attachment);
                std::swap(query_pool, rhs.query_pool);
                enable_query = rhs.enable_query;
            }
            return *this;
        }

        ImguiPass(vk::raii::PhysicalDevice const& physical_device,
                  vk::raii::Device const&         device,
                  SurfaceData&                    surface_data,
                  vk::raii::CommandPool const&    command_pool,
                  vk::raii::Queue const&          queue,
                  DescriptorAllocatorGrowable&    m_descriptor_allocator);

        ~ImguiPass()
        {
            render_pass = nullptr;
            framebuffers.clear();
            m_depth_attachment = nullptr;
        }

        void RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                 vk::raii::Device const&                 device,
                                 vk::raii::CommandBuffer const&          command_buffer,
                                 SurfaceData&                            surface_data,
                                 std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                 vk::Extent2D const&                     extent) override;

        void Start(vk::raii::CommandBuffer const& command_buffer,
                   Meow::SurfaceData const&       surface_data,
                   uint32_t                       current_image_index) override;

        void Draw(vk::raii::CommandBuffer const& command_buffer) override;

    private:
        void CreateGameObjectUI(const std::shared_ptr<GameObject> go);
        void CreateLeafNodeUI(const reflect::refl_shared_ptr<Component> comp_ptr);
        void DrawVecControl(const std::string& label,
                            glm::vec3&         values,
                            float              reset_value  = 0.0f,
                            float              column_width = 100.0f);

        std::unordered_map<std::string, std::function<void(std::string, void*)>> m_editor_ui_creator;
        std::stack<bool>                                                         m_tree_node_open_states;
    };
} // namespace Meow
