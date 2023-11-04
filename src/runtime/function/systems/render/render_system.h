#pragma once

#include "function/systems/system.h"
#include "function/systems/window/window.h"

namespace Meow
{
    struct UBOData
    {
        glm::mat4 model      = glm::mat4(1.0f);
        glm::mat4 view       = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
    };

    class RenderSystem final : public System
    {
    public:
        RenderSystem();
        ~RenderSystem();

        void UpdateUniformBuffer(UBOData ubo_data);
        void Update(float frame_time);

    private:
        void CreateVulkanContent();
        void CreateVulkanInstance();
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        void CreateDebugUtilsMessengerEXT();
#endif
        void CreatePhysicalDevice();
        void CreateSurface();
        void CreateLogicalDevice();
        void CreateCommandPool();
        void CreateCommandBuffers();
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
        void CreateSyncObjects();
        void InitImGui();

        bool StartRenderpass(uint32_t& image_index);
        void EndRenderpass(uint32_t& image_index);
    };
} // namespace Meow
