#pragma once

#include "snake/core/types.hpp"

#include <vulkan/vulkan.h>

namespace snake::gfx {

class Device;

// The graphics pipeline that draws one kind of sprite: the shared vertex shader
// (shaders/sprite.vert) plus the fragment shader for `shape`, and all drawing
// settings, baked into one object. Make one per shape.
class SpritePipeline {
public:
    // colorFormat: the format of the images we draw into (the swapchain's).
    SpritePipeline(const Device& device, VkFormat colorFormat, core::Shape shape);
    ~SpritePipeline();

    SpritePipeline(const SpritePipeline&) = delete;
    SpritePipeline& operator=(const SpritePipeline&) = delete;

    // Record into a command buffer, between vkCmdBeginRendering and vkCmdEndRendering.
    // Bind once, then draw as many sprites of this shape as you like.
    void bind(VkCommandBuffer commandBuffer) const;
    void draw(VkCommandBuffer commandBuffer, const core::Sprite& sprite, VkExtent2D targetSize) const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};

} // namespace snake::gfx
