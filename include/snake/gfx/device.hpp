#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace snake::gfx {

class Instance;
class Surface;

// The GPU we draw with: picks a physical device (the actual GPU), opens a
// logical device (our session with it) and gets the queue we submit work to.
class Device {
public:
    Device(const Instance& instance, const Surface& surface);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    VkPhysicalDevice physical() const { return m_physical; }
    VkDevice handle() const { return m_device; }
    VkQueue queue() const { return m_queue; }
    std::uint32_t queueFamily() const { return m_queueFamily; }

    // Blocks until the GPU has finished all submitted work.
    void waitIdle() const;

private:
    VkPhysicalDevice m_physical = VK_NULL_HANDLE;
    std::uint32_t m_queueFamily = 0;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
};

} // namespace snake::gfx
