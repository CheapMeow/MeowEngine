#pragma once

#include "core/base/non_copyable.h"

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    struct DeferredPass : NonCopyable
    {
        vk::raii::RenderPass render_pass = nullptr;

        DeferredPass(std::nullptr_t) {}

        DeferredPass(vk::raii::Device const& device,
                     vk::Format              color_format,
                     vk::Format              depth_format,
                     vk::AttachmentLoadOp    load_op            = vk::AttachmentLoadOp::eClear,
                     vk::ImageLayout         color_final_layout = vk::ImageLayout::ePresentSrcKHR)
        {
            std::vector<vk::AttachmentDescription> attachment_descriptions;
            assert(color_format != vk::Format::eUndefined);
            attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                                 color_format,
                                                 vk::SampleCountFlagBits::e1,
                                                 load_op,
                                                 vk::AttachmentStoreOp::eStore,
                                                 vk::AttachmentLoadOp::eDontCare,
                                                 vk::AttachmentStoreOp::eDontCare,
                                                 vk::ImageLayout::eUndefined,
                                                 color_final_layout);
            if (depth_format != vk::Format::eUndefined)
            {
                attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                                     depth_format,
                                                     vk::SampleCountFlagBits::e1,
                                                     load_op,
                                                     vk::AttachmentStoreOp::eDontCare,
                                                     vk::AttachmentLoadOp::eDontCare,
                                                     vk::AttachmentStoreOp::eDontCare,
                                                     vk::ImageLayout::eUndefined,
                                                     vk::ImageLayout::eDepthStencilAttachmentOptimal);
            }
            vk::AttachmentReference            color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal);
            vk::AttachmentReference            depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
            vk::SubpassDescription             subpass_description(vk::SubpassDescriptionFlags(),
                                                       vk::PipelineBindPoint::eGraphics,
                                                                   {},
                                                       color_attachment,
                                                                   {},
                                                       (depth_format != vk::Format::eUndefined) ? &depth_attachment :
                                                                                                              nullptr);
            std::vector<vk::SubpassDependency> dependencies;
            dependencies.emplace_back(VK_SUBPASS_EXTERNAL,
                                      0,
                                      vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                      vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                      vk::AccessFlagBits::eNone,
                                      vk::AccessFlagBits::eColorAttachmentWrite);
            vk::RenderPassCreateInfo render_pass_create_info(
                vk::RenderPassCreateFlags(), attachment_descriptions, subpass_description, dependencies);

            render_pass = vk::raii::RenderPass(device, render_pass_create_info);
        }
    };
} // namespace Meow
