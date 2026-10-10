#pragma once

#include "snake/core/types.hpp"

#include <vulkan/vulkan.h>

namespace snake::gfx {

class Device;

// The graphics pipeline that draws solid rectangles (shaders/rect.vert + rect.frag):
// the shaders plus all drawing settings, baked into one object.
class RectPipeline {
public:
    // colorFormat: the format of the images we draw into (the swapchain's).
    RectPipeline(const Device& device, VkFormat colorFormat);
    ~RectPipeline();

    RectPipeline(const RectPipeline&) = delete;
    RectPipeline& operator=(const RectPipeline&) = delete;

    // Record into a command buffer, between vkCmdBeginRendering and vkCmdEndRendering.
    // Bind once, then draw as many rectangles as you like.
    void bind(VkCommandBuffer commandBuffer) const;
    void draw(VkCommandBuffer commandBuffer, const core::Rect& rect, const core::Color& color,
              VkExtent2D targetSize) const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};

} // namespace snake::gfx
