#pragma once

#include "function/systems/render/utils/vulkan_hpp_utils.hpp"
#include "function/systems/system.h"
#include "function/systems/window/window.h"

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <vector>

namespace Meow
{
    struct UBOData
    {
        glm::mat4 model      = glm::mat4(1.0f);
        glm::mat4 view       = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
    };

    struct PerFrameData
    {
        vk::raii::CommandPool   command_pool   = nullptr;
        vk::raii::CommandBuffer command_buffer = nullptr;

        vk::raii::Semaphore image_acquired_semaphore  = nullptr;
        vk::raii::Semaphore render_finished_semaphore = nullptr;
        vk::raii::Fence     in_flight_fence           = nullptr;
    };

    class RenderSystem final : public System
    {
    public:
        RenderSystem();
        ~RenderSystem();

        void UpdateUniformBuffer(UBOData ubo_data);
        void Update(float frame_time);

    private:
        vk::raii::Context  CreateVulkanContent();
        vk::raii::Instance CreateVulkanInstance();
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::raii::DebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT();
#endif
        vk::raii::PhysicalDevice           CreatePhysicalDevice();
        vk::Meow::SurfaceData              CreateSurface();
        vk::raii::Device                   CreateLogicalDevice();
        vk::Meow::SwapChainData            CreateSwapChian();
        vk::Meow::DepthBufferData          CreateDepthBuffer();
        vk::Meow::BufferData               CreateUniformBuffer();
        vk::raii::DescriptorSetLayout      CreateDescriptorSetLayout();
        vk::raii::PipelineLayout           CreatePipelineLayout();
        vk::raii::DescriptorPool           CreateDescriptorPool();
        vk::raii::DescriptorSet            CreateDescriptorSet();
        vk::raii::RenderPass               CreateRenderPass();
        std::vector<vk::raii::Framebuffer> CreateFramebuffers();
        vk::raii::Pipeline                 CreatePipeline();
        std::vector<PerFrameData>          CreatePerFrameData();
        vk::raii::DescriptorPool           InitImGui();

        bool StartRenderpass();
        void EndRenderpass();

        uint32_t m_current_frame_index = 0;
        uint32_t m_current_image_index = 0;

        vk::raii::Context  m_vulkan_context;
        vk::raii::Instance m_vulkan_instance;
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::raii::DebugUtilsMessengerEXT m_debug_utils_messenger;
#endif
        vk::raii::PhysicalDevice           m_gpu;
        vk::Meow::SurfaceData              m_surface_data;
        uint32_t                           m_graphics_queue_family_index;
        uint32_t                           m_present_queue_family_index;
        vk::raii::Device                   m_logical_device;
        vk::raii::Queue                    m_graphics_queue;
        vk::raii::Queue                    m_present_queue;
        vk::Meow::SwapChainData            m_swapchain_data;
        vk::Meow::DepthBufferData          m_depth_buffer_data;
        vk::Meow::BufferData               m_uniform_buffer_data;
        vk::raii::DescriptorSetLayout      m_descriptor_set_layout;
        vk::raii::PipelineLayout           m_pipeline_layout;
        vk::raii::DescriptorPool           m_descriptor_pool;
        vk::raii::DescriptorSet            m_descriptor_set;
        vk::raii::RenderPass               m_render_pass;
        std::vector<vk::raii::Framebuffer> m_framebuffers;
        vk::raii::Pipeline                 m_graphics_pipeline;
        std::vector<PerFrameData>          m_per_frame_data;
        vk::raii::DescriptorPool           m_imgui_descriptor_pool;
    };
} // namespace Meow
