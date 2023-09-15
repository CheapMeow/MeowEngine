#pragma once

#include "core/base/non_copyable.h"
#include "function/renderer/utils/vulkan_hpp_utils.hpp"
#include "function/renderer/window.h"

#include <volk.h>
#include <vulkan/vulkan_raii.hpp>

#include "core/base/macro.h"

#include <memory>

namespace Meow
{
    class VulkanRenderer : NonCopyable
    {
    public:
        VulkanRenderer() = delete;

        VulkanRenderer(std::shared_ptr<Window>               window,
                       vk::raii::Context&                    vulkan_context,
                       vk::raii::Instance&                   vulkan_instance,
                       vk::raii::PhysicalDevice&             gpu,
                       vk::Meow::SurfaceData&                surface_data,
                       uint32_t                              graphics_queue_family_index,
                       uint32_t                              present_queue_family_index,
                       vk::raii::Device&                     logical_device,
                       vk::raii::Queue&                      graphics_queue,
                       vk::raii::Queue&                      present_queue,
                       vk::raii::CommandPool&                command_pool,
                       std::vector<vk::raii::CommandBuffer>& command_buffers,
                       vk::Meow::SwapChainData&              swapchain_data,
                       vk::Meow::DepthBufferData&            depth_buffer_data,
                       vk::Meow::BufferData&                 uniform_buffer_data,
                       vk::raii::DescriptorSetLayout&        descriptor_set_layout,
                       vk::raii::PipelineLayout&             pipeline_layout,
                       vk::raii::DescriptorPool&             descriptor_pool,
                       vk::raii::DescriptorSet&              descriptor_set,
                       vk::raii::RenderPass&                 render_pass,
                       vk::raii::ShaderModule&               vertex_shader_module,
                       vk::raii::ShaderModule&               fragment_shader_module,
                       std::vector<vk::raii::Framebuffer>&   framebuffers,
                       vk::Meow::BufferData&                 vertex_buffer_data,
                       vk::raii::Pipeline&                   graphics_pipeline
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
                       ,
                       vk::raii::DebugUtilsMessengerEXT& debug_utils_messenger
#endif
        );

        VulkanRenderer(VulkanRenderer&& rhs) VULKAN_HPP_NOEXCEPT;

        ~VulkanRenderer();

        static VulkanRenderer CreateRenderer(std::shared_ptr<Window> window);

        void Init();

        void Update();

    private:
        bool StartRenderpass(uint32_t& image_index);
        void EndRenderpass(uint32_t& image_index);

        std::weak_ptr<Window>                             m_window;
        vk::raii::Context                                 m_vulkan_context;
        vk::raii::Instance                                m_vulkan_instance;
        vk::raii::PhysicalDevice                          m_gpu;
        vk::Meow::SurfaceData                             m_surface_data;
        uint32_t                                          m_graphics_queue_family_index;
        uint32_t                                          m_present_queue_family_index;
        vk::raii::Device                                  m_logical_device;
        vk::raii::Queue                                   m_graphics_queue;
        vk::raii::Queue                                   m_present_queue;
        vk::raii::CommandPool                             m_command_pool;
        std::vector<vk::raii::CommandBuffer>              m_command_buffers;
        vk::Meow::SwapChainData                           m_swapchain_data;
        vk::Meow::DepthBufferData                         m_depth_buffer_data;
        vk::Meow::BufferData                              m_uniform_buffer_data;
        vk::raii::DescriptorSetLayout                     m_descriptor_set_layout;
        vk::raii::PipelineLayout                          m_pipeline_layout;
        vk::raii::DescriptorPool                          m_descriptor_pool;
        vk::raii::DescriptorSet                           m_descriptor_set;
        vk::raii::RenderPass                              m_render_pass;
        vk::raii::ShaderModule                            m_vertex_shader_module;
        vk::raii::ShaderModule                            m_fragment_shader_module;
        std::vector<vk::raii::Framebuffer>                m_framebuffers;
        vk::Meow::BufferData                              m_vertex_buffer_data;
        vk::raii::Pipeline                                m_graphics_pipeline;
        std::vector<std::shared_ptr<vk::raii::Semaphore>> m_image_acquired_semaphores;
        std::vector<std::shared_ptr<vk::raii::Semaphore>> m_render_finished_semaphores;
        std::vector<std::shared_ptr<vk::raii::Fence>>     m_in_flight_fences;
        uint32_t                                          m_current_frame_index = 0;

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::raii::DebugUtilsMessengerEXT m_debug_utils_messenger;
#endif
    };
} // namespace Meow
