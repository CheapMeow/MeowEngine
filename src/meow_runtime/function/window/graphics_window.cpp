#include "graphics_window.h"

#include "function/global/runtime_context.h"

#include "function/render/utils/vulkan_debug_utils.h"

namespace Meow
{
    void GraphicsWindow::CreateSurface()
    {
        const vk::raii::Instance&       vulkan_instance = g_runtime_context.render_system->GetInstance();
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();

        auto         size = GetSize();
        vk::Extent2D extent(size.x, size.y);
        m_surface_data = SurfaceData(vulkan_instance, GetGLFWWindow(), extent);

        m_color_format = PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*m_surface_data.surface)).format;
        ASSERT(m_color_format != vk::Format::eUndefined);
    }

    void GraphicsWindow::CreateSwapChian()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        m_swapchain_data =
            SwapChainData(physical_device,
                          logical_device,
                          m_surface_data.surface,
                          m_surface_data.extent,
                          vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                          nullptr,
                          g_runtime_context.render_system->GetGraphicsQueueFamiliyIndex(),
                          g_runtime_context.render_system->GetPresentQueueFamilyIndex());

        m_swapchain_image_views.resize(m_swapchain_data.image_views.size());
        for (int i = 0; i < m_swapchain_data.image_views.size(); i++)
        {
            m_swapchain_image_views[i] = *m_swapchain_data.image_views[i];
        }

        m_swapchain_image_number = m_swapchain_data.image_views.size();
    }

    void GraphicsWindow::CreatePerFrameData()
    {
        const vk::raii::Device&      logical_device = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::Queue&       graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();
        const vk::raii::CommandPool& command_pool   = g_runtime_context.render_system->GetCommandPool();
        const auto graphics_queue_family_index      = g_runtime_context.render_system->GetGraphicsQueueFamiliyIndex();
        const auto k_max_frames_in_flight           = g_runtime_context.render_system->GetMaxFramesInFlight();

        m_per_frame_data.resize(k_max_frames_in_flight);
        for (auto& frame_data : m_per_frame_data)
        {
            // graphics

            vk::CommandBufferAllocateInfo command_buffer_allocate_info(
                *command_pool, vk::CommandBufferLevel::ePrimary, 1);
            vk::raii::CommandBuffers command_buffers(logical_device, command_buffer_allocate_info);
            frame_data.graphics_command_buffer = std::move(command_buffers[0]);

            frame_data.graphics_in_flight_fence =
                vk::raii::Fence(logical_device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));

            // compute

            vk::CommandBufferAllocateInfo compute_command_buffer_allocate_info(
                *command_pool, vk::CommandBufferLevel::ePrimary, 1);
            vk::raii::CommandBuffers compute_command_buffers(logical_device, compute_command_buffer_allocate_info);
            frame_data.compute_command_buffer = std::move(compute_command_buffers[0]);

            frame_data.compute_finished_semaphore = vk::raii::Semaphore(logical_device, vk::SemaphoreCreateInfo());
            frame_data.compute_in_flight_fence =
                vk::raii::Fence(logical_device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
        }

        m_per_image_data.resize(m_swapchain_image_number);
        for (auto& image_data : m_per_image_data)
        {
            image_data.present_finished_semaphore = vk::raii::Semaphore(logical_device, vk::SemaphoreCreateInfo());
            image_data.render_finished_semaphore  = vk::raii::Semaphore(logical_device, vk::SemaphoreCreateInfo());
        }

        SetDebugName();
    }

    void GraphicsWindow::SetDebugName() const
    {
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        const vk::raii::Device& logical_device         = g_runtime_context.render_system->GetLogicalDevice();
        const auto              k_max_frames_in_flight = g_runtime_context.render_system->GetMaxFramesInFlight();

        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
        {
            {
                std::string command_buffer_name = std::format("PerFrameData graphics command buffer {}", i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eCommandBuffer,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkCommandBuffer,
                                                           *m_per_frame_data[i].graphics_command_buffer),
                    command_buffer_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string fence_name = std::format("PerFrameData in graphics flight fence {}", i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eFence,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkFence, *m_per_frame_data[i].graphics_in_flight_fence),
                    fence_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string command_buffer_name = std::format("PerFrameData compute command buffer {}", i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eCommandBuffer,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkCommandBuffer,
                                                           *m_per_frame_data[i].compute_command_buffer),
                    command_buffer_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string semaphore_name = std::format("PerFrameData compute finished semaphore {}", i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eSemaphore,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkSemaphore,
                                                           *m_per_frame_data[i].compute_finished_semaphore),
                    semaphore_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string fence_name = std::format("PerFrameData in compute flight fence {}", i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eFence,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkFence, *m_per_frame_data[i].compute_in_flight_fence),
                    fence_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }
        }

        for (uint32_t i = 0; i < m_swapchain_image_number; ++i)
        {
            {
                std::string semaphore_name = std::format("PerSwapChainImageData image acquired semaphore {}", i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eSemaphore,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkSemaphore,
                                                           *m_per_image_data[i].present_finished_semaphore),
                    semaphore_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string semaphore_name = std::format("PerSwapChainImageData render finished semaphore {}", i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eSemaphore,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkSemaphore, *m_per_image_data[i].render_finished_semaphore),
                    semaphore_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }
        }
#endif
    }
} // namespace Meow