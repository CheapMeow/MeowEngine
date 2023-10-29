#pragma once

#include "core/base/non_copyable.h"
#include "function/renderer/utils/vulkan_hpp_utils.hpp"
#include "function/renderer/window.h"

// TODO: temp
#include "function/renderer/structs/model.h"

#include <volk.h>
#include <vulkan/vulkan_raii.hpp>

#include "core/base/macro.h"

#include <memory>

namespace Meow
{
    struct UBOData
    {
        glm::mat4 model      = glm::mat4(1.0f);
        glm::mat4 view       = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
    };

    class VulkanRenderer : NonCopyable
    {
        friend class RenderSystem;

    public:
        VulkanRenderer();

        ~VulkanRenderer();

        void UpdateUniformBuffer(UBOData ubo_data);

    private:
        vk::raii::Context                    CreateVulkanContent();
        vk::raii::Instance                   CreateVulkanInstance();
        vk::raii::PhysicalDevice             CreatePhysicalDevice();
        vk::Meow::SurfaceData                CreateSurface();
        vk::raii::Device                     CreateLogicalDevice();
        vk::raii::CommandPool                CreateCommandPool();
        std::vector<vk::raii::CommandBuffer> CreateCommandBuffers();
        vk::Meow::SwapChainData              CreateSwapChian();
        vk::Meow::DepthBufferData            CreateDepthBuffer();
        vk::Meow::BufferData                 CreateUniformBuffer();
        vk::raii::DescriptorSetLayout        CreateDescriptorSetLayout();
        vk::raii::PipelineLayout             CreatePipelineLayout();
        vk::raii::DescriptorPool             CreateDescriptorPool();
        vk::raii::DescriptorSet              CreateDescriptorSet();
        vk::raii::RenderPass                 CreateRenderPass();
        std::vector<vk::raii::Framebuffer>   CreateFramebuffers();
        vk::Meow::BufferData                 CreateVertexBuffer();
        vk::raii::Pipeline                   CreatePipeline();
        void                                 CreateSyncObjects();

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::raii::DebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT();
#endif

        bool StartRenderpass(uint32_t& image_index);
        void EndRenderpass(uint32_t& image_index);

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
