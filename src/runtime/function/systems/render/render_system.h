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
        void CreateVulkanInstance();
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        void CreateDebugUtilsMessengerEXT();
#endif
        void CreatePhysicalDevice();
        void CreateSurface();
        void CreateLogicalDevice();
        void CreateSwapChian();
        void CreateDepthBuffer();
        void CreateUniformBuffer();
        void CreateDescriptorSetLayout();
        void CreatePipelineLayout();
        void CreateDescriptorPool();
        void CreateDescriptorSet();
        void CreateRenderPass();
        void CreateFramebuffers();
        void CreatePipeline();
        void CreatePerFrameData();
        void InitImGui();

        bool StartRenderpass();
        void EndRenderpass();

        uint32_t m_current_frame_index = 0;
        uint32_t m_current_image_index = 0;

        uint32_t m_graphics_queue_family_index = 0;
        uint32_t m_present_queue_family_index  = 0;

        vk::raii::Context  m_vulkan_context;
        vk::raii::Instance m_vulkan_instance = nullptr;
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::raii::DebugUtilsMessengerEXT m_debug_utils_messenger = nullptr;
#endif
        vk::raii::PhysicalDevice           m_gpu                   = nullptr;
        vk::Meow::SurfaceData              m_surface_data          = nullptr;
        vk::raii::Device                   m_logical_device        = nullptr;
        vk::raii::Queue                    m_graphics_queue        = nullptr;
        vk::raii::Queue                    m_present_queue         = nullptr;
        vk::Meow::SwapChainData            m_swapchain_data        = nullptr;
        vk::Meow::DepthBufferData          m_depth_buffer_data     = nullptr;
        vk::Meow::BufferData               m_uniform_buffer_data   = nullptr;
        vk::raii::DescriptorSetLayout      m_descriptor_set_layout = nullptr;
        vk::raii::PipelineLayout           m_pipeline_layout       = nullptr;
        vk::raii::DescriptorPool           m_descriptor_pool       = nullptr;
        vk::raii::DescriptorSet            m_descriptor_set        = nullptr;
        vk::raii::RenderPass               m_render_pass           = nullptr;
        std::vector<vk::raii::Framebuffer> m_framebuffers;
        vk::raii::Pipeline                 m_graphics_pipeline = nullptr;
        std::vector<PerFrameData>          m_per_frame_data;

        vk::raii::DescriptorPool m_imgui_descriptor_pool = nullptr;
    };
} // namespace Meow
