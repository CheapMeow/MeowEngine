#include "core/base/non_copyable.h"
#include "function/renderer/window.h"

#include <volk.h>
#include <vulkan/vulkan_raii.hpp>

#include "core/base/macro.h"

#include <memory>

namespace Meow
{
    class LIBRARY_API VulkanRenderer : NonCopyable
    {
    public:
        VulkanRenderer(std::shared_ptr<Window> window);

        ~VulkanRenderer();

    private:
        void CreateContext();
        void CreateInstance(std::vector<const char*> const& required_instance_extensions,
                            std::vector<const char*> const& required_validation_layers);
        void CreatePhysicalDevice();
        void CreateSurface();
        void CreateLogicalDevice();

        const std::vector<const char*> k_required_device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        std::weak_ptr<Window>                     m_window;
        std::shared_ptr<vk::raii::Context>        m_vulkan_context;
        std::shared_ptr<vk::raii::Instance>       m_vulkan_instance;
        std::shared_ptr<vk::raii::PhysicalDevice> m_gpu;
        std::shared_ptr<vk::raii::SurfaceKHR>     m_surface;
        uint32_t                                  m_graphics_queue_family_index;
        uint32_t                                  m_present_queue_family_index;
        std::shared_ptr<vk::raii::Device>         m_logical_device;

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::shared_ptr<vk::raii::DebugUtilsMessengerEXT> m_debug_utils_messenger;
#endif
    };
} // namespace Meow
