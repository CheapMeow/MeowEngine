#pragma once

#include "core/base/bitmask.hpp"
#include "function/render/allocator/descriptor_allocator_growable.h"
#include "function/render/buffer_data/image_data.h"
#include "function/render/model/model.hpp"
#include "function/system.h"
#include "function/window/window.h"

#include <vulkan/vulkan_raii.hpp>

#include <functional>
#include <memory>
#include <queue>
#include <vector>

namespace Meow
{
    class RenderSystem final : public System
    {
    public:
        RenderSystem();
        ~RenderSystem();

        void Start() override {}

        void Tick(float dt) override {}

        const vk::raii::Instance&       GetInstance() const { return m_vulkan_instance; };
        const vk::raii::PhysicalDevice& GetPhysicalDevice() const { return m_physical_device; }
        const vk::raii::Device&         GetLogicalDevice() const { return m_logical_device; }
        const vk::raii::Queue&          GetGraphicsQueue() const { return m_graphics_queue; }
        const vk::raii::Queue&          GetPresentQueue() const { return m_present_queue; }
        const vk::raii::Queue&          GetComputeQueue() const { return m_compute_queue; }
        const vk::raii::CommandPool&    GetOneTimeSubmitCommandPool() const { return m_onetime_submit_command_pool; }
        DescriptorAllocatorGrowable&    GetDescriptorAllocator() { return m_descriptor_allocator; }

        const uint32_t                GetGraphicsQueueFamiliyIndex() const { return m_graphics_queue_family_index; }
        const uint32_t                GetPresentQueueFamilyIndex() const { return m_present_queue_family_index; }
        const vk::SampleCountFlagBits GetMSAASamples() const { return m_msaa_samples; }
        const bool     GetDepthWritebackResolveSupported() const { return m_depth_writeback_resolve_supported; }
        const bool     GetResolveDepthOnWriteback() const { return m_resolve_depth_on_writeback; }
        const bool     GetPostProcessRunning() const { return m_postprocess_running; }
        const uint32_t GetMaxFramesInFlight() const { return k_max_frames_in_flight; }

    private:
        void CreateVulkanInstance();
        void CreatePhysicalDevice();
        void CreateLogicalDevice();
        void CreateCommandPool();
        void CreateDescriptorAllocator();

        vk::raii::Context           m_vulkan_context;
        vk::raii::Instance          m_vulkan_instance             = nullptr;
        vk::raii::PhysicalDevice    m_physical_device             = nullptr;
        vk::raii::Device            m_logical_device              = nullptr;
        vk::raii::Queue             m_graphics_queue              = nullptr;
        vk::raii::Queue             m_present_queue               = nullptr;
        vk::raii::Queue             m_compute_queue               = nullptr;
        vk::raii::CommandPool       m_onetime_submit_command_pool = nullptr;
        DescriptorAllocatorGrowable m_descriptor_allocator        = nullptr;

        vk::SampleCountFlagBits m_msaa_samples;

        /**
         * @brief If true, the platform supports the VK_KHR_depth_stencil_resolve extension
         *        and therefore can resolve the depth attachment on writeback
         */
        bool m_depth_writeback_resolve_supported = false;

        /**
         * @brief If the platform supports the VK_KHR_depth_stencil_resolve extension,
         *        then the list of supported depth resolve mode.
         */
        std::vector<vk::ResolveModeFlagBits> m_supported_depth_resolve_mode_list;

        /**
         * @brief If the platform supports the VK_KHR_depth_stencil_resolve extension,
         *        then current depth resolve mode.
         */
        vk::ResolveModeFlagBits m_current_depth_resolve_mode;

        /**
         * @brief If true, enable writeback depth resolve.
         *        If false, the multisampled depth attachment will be stored
         *        (only if postprocessing is enabled since the attachment is
         *        otherwise unused).
         *        Writeback resolve means that the multisampled attachment
         *        can be discarded at the end of the render pass.
         */
        bool m_resolve_depth_on_writeback = true;

        /**
         * @brief If true, some post processes are runing. If false, no post process is running.
         *        It will affect other settings such as whether resolve depth on writeback or not.
         *
         */
        bool m_postprocess_running = false;

        uint32_t m_graphics_queue_family_index = 0;
        uint32_t m_present_queue_family_index  = 0;
        uint32_t m_compute_queue_family_index  = 0;

        const uint32_t k_max_frames_in_flight = 2;
    };
} // namespace Meow
