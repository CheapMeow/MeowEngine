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
    }

    void GraphicsWindow::CreatePerFrameData()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::Queue&  graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();
        const auto graphics_queue_family_index = g_runtime_context.render_system->GetGraphicsQueueFamiliyIndex();
        const auto k_max_frames_in_flight      = g_runtime_context.render_system->GetMaxFramesInFlight();

        m_per_frame_data.resize(k_max_frames_in_flight);
        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
        {
            vk::CommandPoolCreateInfo command_pool_create_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                               graphics_queue_family_index);
            m_per_frame_data[i].command_pool = vk::raii::CommandPool(logical_device, command_pool_create_info);

            // graphics

            vk::CommandBufferAllocateInfo command_buffer_allocate_info(
                *m_per_frame_data[i].command_pool, vk::CommandBufferLevel::ePrimary, 1);
            vk::raii::CommandBuffers command_buffers(logical_device, command_buffer_allocate_info);
            m_per_frame_data[i].graphics_command_buffer = std::move(command_buffers[0]);

            m_per_frame_data[i].image_acquired_semaphore =
                vk::raii::Semaphore(logical_device, vk::SemaphoreCreateInfo());
            m_per_frame_data[i].render_finished_semaphore =
                vk::raii::Semaphore(logical_device, vk::SemaphoreCreateInfo());
            m_per_frame_data[i].graphics_in_flight_fence =
                vk::raii::Fence(logical_device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));

            // compute

            vk::CommandBufferAllocateInfo compute_command_buffer_allocate_info(
                *m_per_frame_data[i].command_pool, vk::CommandBufferLevel::ePrimary, 1);
            vk::raii::CommandBuffers compute_command_buffers(logical_device, compute_command_buffer_allocate_info);
            m_per_frame_data[i].compute_command_buffer = std::move(compute_command_buffers[0]);

            m_per_frame_data[i].compute_finished_semaphore =
                vk::raii::Semaphore(logical_device, vk::SemaphoreCreateInfo());
            m_per_frame_data[i].compute_in_flight_fence =
                vk::raii::Fence(logical_device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
        }

        SetPerFrameDataDebugName();
    }

    void GraphicsWindow::SetPerFrameDataDebugName() const
    {
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        const vk::raii::Device& logical_device         = g_runtime_context.render_system->GetLogicalDevice();
        const auto              k_max_frames_in_flight = g_runtime_context.render_system->GetMaxFramesInFlight();

        const std::string debug_name = "PerFrameData";

        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
        {
            {
                std::string command_pool_name = std::format("{} command pool frame {}", debug_name, i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eCommandPool,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkCommandPool, *m_per_frame_data[i].command_pool),
                    command_pool_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string command_buffer_name = std::format("{} graphics command buffer frame {}", debug_name, i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eCommandBuffer,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkCommandBuffer,
                                                           *m_per_frame_data[i].graphics_command_buffer),
                    command_buffer_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string semaphore_name = std::format("{} image acquired semaphore frame {}", debug_name, i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eSemaphore,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkSemaphore, *m_per_frame_data[i].image_acquired_semaphore),
                    semaphore_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string semaphore_name = std::format("{} render finished semaphore frame {}", debug_name, i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eSemaphore,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkSemaphore, *m_per_frame_data[i].render_finished_semaphore),
                    semaphore_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string fence_name = std::format("{} in graphics flight fence frame {}", debug_name, i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eFence,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkFence, *m_per_frame_data[i].graphics_in_flight_fence),
                    fence_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string command_buffer_name = std::format("{} compute command buffer frame {}", debug_name, i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eCommandBuffer,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkCommandBuffer,
                                                           *m_per_frame_data[i].compute_command_buffer),
                    command_buffer_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string semaphore_name = std::format("{} compute finished semaphore frame {}", debug_name, i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eSemaphore,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkSemaphore,
                                                           *m_per_frame_data[i].compute_finished_semaphore),
                    semaphore_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }

            {
                std::string fence_name = std::format("{} in compute flight fence frame {}", debug_name, i);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eFence,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkFence, *m_per_frame_data[i].compute_in_flight_fence),
                    fence_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }
        }
#endif
    }
} // namespace Meow