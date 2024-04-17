#pragma once

#include "function/render/structs/buffer_data.h"
#include "function/render/structs/depth_buffer_data.h"
#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"
#include "function/render/structs/surface_data.h"
#include "function/render/structs/swapchain_data.h"
#include "function/render/structs/texture_data.hpp"
#include "function/render/structs/ubo_data.h"
#include "function/system.h"
#include "function/window/window.h"
#include "render_pass/deferred_pass.h"

#include <vulkan/vulkan_raii.hpp>

#include <functional>
#include <memory>
#include <queue>
#include <vector>

namespace Meow
{
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

        void Start() override;

        void Update(float frame_time);

        void SetResized(bool resized) { m_framebuffer_resized = resized; }

        TextureData CreateTextureData(vk::Extent2D const& extent)
        {
            return TextureData(m_gpu, m_logical_device, extent);
        }

        /**
         * @brief Commands can only be recorded into VkCommandBuffer after VkCommandBuffer.begin().
         * So when out of range of VkCommandBuffer.begin() and .end(), store commands into a pending queue.
         *
         * @param command Function handler of render task.
         */
        void EnQueueRenderCommand(std::function<void(vk::raii::CommandBuffer const&)> command);

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
        void CreateDescriptorAllocator();
        void CreateRenderPass();
        void CreatePerFrameData();
        void InitImGui();
        void RecreateSwapChain();

        bool StartRenderpass();
        void EndRenderpass();

        const std::vector<const char*> k_required_device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        const uint64_t                 k_fence_timeout              = 100000000;
        const uint32_t                 k_max_frames_in_flight       = 2;

        uint32_t m_current_frame_index = 0;
        uint32_t m_current_image_index = 0;

        uint32_t m_graphics_queue_family_index = 0;
        uint32_t m_present_queue_family_index  = 0;

        bool m_framebuffer_resized = false;

        vk::raii::Context  m_vulkan_context;
        vk::raii::Instance m_vulkan_instance = nullptr;
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::raii::DebugUtilsMessengerEXT m_debug_utils_messenger = nullptr;
#endif
        vk::raii::PhysicalDevice           m_gpu                  = nullptr;
        SurfaceData                        m_surface_data         = nullptr;
        vk::raii::Device                   m_logical_device       = nullptr;
        vk::raii::Queue                    m_graphics_queue       = nullptr;
        vk::raii::Queue                    m_present_queue        = nullptr;
        SwapChainData                      m_swapchain_data       = nullptr;
        UploadContext                      m_upload_context       = nullptr;
        DepthBufferData                    m_depth_buffer_data    = nullptr;
        DescriptorAllocatorGrowable        m_descriptor_allocator = nullptr;
        DeferredPass                       m_deferred_pass        = nullptr;
        std::vector<PerFrameData>          m_per_frame_data;

        // TODO: Dynamic descriptor pool?
        vk::raii::DescriptorPool m_imgui_descriptor_pool = nullptr;

        // TODO: temp shader
        Material testMat = nullptr;

        // TODO: temp texture
        std::shared_ptr<TextureData> diffuse_texture = nullptr;

        /**
         * @brief Commands can only be recorded into VkCommandBuffer after VkCommandBuffer.begin().
         * So when out of range of VkCommandBuffer.begin() and .end(), store commands into a pending queue.
         */
        std::queue<std::function<void(vk::raii::CommandBuffer const&)>> m_pending_commands;
    };
} // namespace Meow
