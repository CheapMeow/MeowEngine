#pragma once

#include "function/systems/render/utils/vulkan_hpp_utils.hpp"
#include "function/systems/window/window.h"

#include <vulkan/vulkan_raii.hpp>

#include <memory>

namespace Meow
{
    struct RenderContext
    {
        std::shared_ptr<Window> window;

        std::shared_ptr<vk::raii::Context>  vulkan_context;
        std::shared_ptr<vk::raii::Instance> vulkan_instance;
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::shared_ptr<vk::raii::DebugUtilsMessengerEXT> debug_utils_messenger;
#endif
        std::shared_ptr<vk::raii::PhysicalDevice>         physical_device;
        std::shared_ptr<vk::Meow::SurfaceData>            surface_data;
        uint32_t                                          graphics_queue_family_index;
        uint32_t                                          present_queue_family_index;
        std::shared_ptr<vk::raii::Device>                 logical_device;
        std::shared_ptr<vk::raii::Queue>                  graphics_queue;
        std::shared_ptr<vk::raii::Queue>                  present_queue;
        std::shared_ptr<vk::raii::CommandPool>            command_pool;
        std::vector<vk::raii::CommandBuffer>              command_buffers;
        std::shared_ptr<vk::Meow::SwapChainData>          swapchain_data;
        std::shared_ptr<vk::Meow::DepthBufferData>        depth_buffer_data;
        std::shared_ptr<vk::Meow::BufferData>             uniform_buffer_data;
        std::shared_ptr<vk::raii::DescriptorSetLayout>    descriptor_set_layout;
        std::shared_ptr<vk::raii::PipelineLayout>         pipeline_layout;
        std::shared_ptr<vk::raii::DescriptorPool>         descriptor_pool;
        std::shared_ptr<vk::raii::DescriptorSet>          descriptor_set;
        std::shared_ptr<vk::raii::RenderPass>             render_pass;
        std::vector<vk::raii::Framebuffer>                framebuffers;
        std::shared_ptr<vk::raii::Pipeline>               graphics_pipeline;
        std::vector<std::shared_ptr<vk::raii::Semaphore>> image_acquired_semaphores;
        std::vector<std::shared_ptr<vk::raii::Semaphore>> render_finished_semaphores;
        std::vector<std::shared_ptr<vk::raii::Fence>>     in_flight_fences;
        uint32_t                                          current_frame_index = 0;

        std::shared_ptr<vk::raii::DescriptorPool> imgui_descriptor_pool;
    };

} // namespace Meow