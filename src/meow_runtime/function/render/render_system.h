#pragma once

#include "core/base/bitmask.hpp"
#include "function/render/render_resources/image_data.h"
#include "function/render/render_resources/model.hpp"
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

        void Start() override;

        void Tick(float dt) override;

        const vk::raii::Instance&       GetInstance() const { return m_vulkan_instance; };
        const vk::raii::PhysicalDevice& GetPhysicalDevice() const { return m_physical_device; }
        const vk::raii::Device&         GetLogicalDevice() const { return m_logical_device; }
        const uint32_t                  GetGraphicsQueueFamiliyIndex() const { return m_graphics_queue_family_index; }
        const uint32_t                  GetPresentQueueFamilyIndex() const { return m_present_queue_family_index; }
        const vk::raii::CommandPool&    GetOneTimeSubmitCommandPool() const { return m_onetime_submit_command_pool; }
        const vk::raii::Queue&          GetGraphicsQueue() const { return m_graphics_queue; }
        const vk::raii::Queue&          GetPresentQueue() const { return m_present_queue; }

    private:
        void CreateVulkanInstance();
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        void CreateDebugUtilsMessengerEXT();
#endif
        void CreatePhysicalDevice();
        void CreateLogicalDevice();

        uint32_t m_graphics_queue_family_index = 0;
        uint32_t m_present_queue_family_index  = 0;

        vk::raii::Context  m_vulkan_context;
        vk::raii::Instance m_vulkan_instance = nullptr;
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::raii::DebugUtilsMessengerEXT m_debug_utils_messenger = nullptr;
#endif
        vk::raii::PhysicalDevice m_physical_device             = nullptr;
        vk::raii::Device         m_logical_device              = nullptr;
        vk::raii::Queue          m_graphics_queue              = nullptr;
        vk::raii::Queue          m_present_queue               = nullptr;
        vk::raii::CommandPool    m_onetime_submit_command_pool = nullptr;
    };
} // namespace Meow
