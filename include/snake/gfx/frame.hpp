#pragma once

#include <vulkan/vulkan.h>

namespace snake::gfx {

class Device;

// What one in-flight frame needs for itself. While the GPU works on one frame,
// the CPU can already record the next one into a different Frame.
class Frame {
public:
    explicit Frame(const Device& device);
    ~Frame();

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;

    // Blocks until the GPU finished the commands last submitted from this frame.
    void waitUntilFree() const;

    // Makes the fence and command buffer ready for reuse. Only call when you're
    // sure you'll submit again: the next waitUntilFree waits for that submit.
    void reset() const;

    VkCommandBuffer commandBuffer() const { return m_commandBuffer; }
    VkSemaphore imageAcquired() const { return m_imageAcquired; } // GPU: swapchain image is free
    VkFence inFlight() const { return m_inFlight; }               // CPU: GPU finished this frame

private:
    void destroy();

    VkDevice m_device = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    VkSemaphore m_imageAcquired = VK_NULL_HANDLE;
    VkFence m_inFlight = VK_NULL_HANDLE;
};

} // namespace snake::gfx
