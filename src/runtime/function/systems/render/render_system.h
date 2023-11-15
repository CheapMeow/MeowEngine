#pragma once

#include "function/systems/render/structs/buffer_data.h"
#include "function/systems/render/structs/depth_buffer_data.h"
#include "function/systems/render/structs/surface_data.h"
#include "function/systems/render/structs/swapchain_data.h"
#include "function/systems/render/structs/texture_data.h"
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

        PerFrameData() {}

        PerFrameData(std::nullptr_t) {}
    };

    /**
     * @brief Using single command pool is intending to
     * reseting command pool to reset all command buffers.
     *
     * But now in one time submit function, it create command buffer, submit to queue, and then delete it.
     *
     * So use which method is a question, need to be tested in later time.
     *
     * You can also allocate a fence to wait for all command buffers in upload context,
     * but it is also pending to discuss later.
     */
    struct UploadContext
    {
        vk::raii::CommandPool command_pool = nullptr;

        UploadContext() {}

        UploadContext(std::nullptr_t) {}
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
        void CreateUploadContext();
        void CreateDepthBuffer();
        void CreateUniformBuffer();
        void CreateDescriptorSetLayout();
        void CreateDescriptorPool();
        void CreateDescriptorSet();
        void CreateRenderPass();
        void CreateFramebuffers();
        void CreatePerFrameData();
        void InitImGui();

        bool StartRenderpass();
        void EndRenderpass();

        const std::vector<const char*> k_required_device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        const uint64_t                 k_fence_timeout              = 100000000;
        const uint32_t                 k_max_frames_in_flight       = 2;

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
        SurfaceData                        m_surface_data          = nullptr;
        vk::raii::Device                   m_logical_device        = nullptr;
        vk::raii::Queue                    m_graphics_queue        = nullptr;
        vk::raii::Queue                    m_present_queue         = nullptr;
        SwapChainData                      m_swapchain_data        = nullptr;
        UploadContext                      m_upload_context        = nullptr;
        DepthBufferData                    m_depth_buffer_data     = nullptr;
        BufferData                         m_uniform_buffer_data   = nullptr;
        vk::raii::DescriptorSetLayout      m_descriptor_set_layout = nullptr;
        vk::raii::DescriptorPool           m_descriptor_pool       = nullptr;
        vk::raii::DescriptorSet            m_descriptor_set        = nullptr;
        vk::raii::RenderPass               m_render_pass           = nullptr;
        std::vector<vk::raii::Framebuffer> m_framebuffers;
        std::vector<PerFrameData>          m_per_frame_data;

        vk::raii::DescriptorPool m_imgui_descriptor_pool = nullptr;
    };
} // namespace Meow
