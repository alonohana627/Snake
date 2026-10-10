#include "snake/gfx/frame.hpp"

#include "snake/gfx/check.hpp"
#include "snake/gfx/device.hpp"

#include <cstdint>

namespace snake::gfx {

Frame::Frame(const Device& device)
    : m_device(device.handle())
{
    // If a step throws, the destructor won't run (the object never finished
    // constructing), so clean up what was already made ourselves.
    try {
        const VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = device.queueFamily(),
        };
        check(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool), "vkCreateCommandPool");

        const VkCommandBufferAllocateInfo allocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        check(vkAllocateCommandBuffers(m_device, &allocateInfo, &m_commandBuffer), "vkAllocateCommandBuffers");

        const VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        check(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAcquired), "vkCreateSemaphore");

        // Start signaled, so waiting on it in the very first frame doesn't block forever.
        const VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        check(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlight), "vkCreateFence");
    } catch (...) {
        destroy();
        throw;
    }
}

Frame::~Frame()
{
    destroy();
}

void Frame::waitUntilFree() const
{
    check(vkWaitForFences(m_device, 1, &m_inFlight, VK_TRUE, UINT64_MAX), "vkWaitForFences");
}

void Frame::reset() const
{
    check(vkResetFences(m_device, 1, &m_inFlight), "vkResetFences");
    check(vkResetCommandPool(m_device, m_commandPool, 0), "vkResetCommandPool");
}

// Safe on a partly built Frame: destroying VK_NULL_HANDLE is allowed and does nothing.
void Frame::destroy()
{
    vkDestroyFence(m_device, m_inFlight, nullptr);
    vkDestroySemaphore(m_device, m_imageAcquired, nullptr);
    vkDestroyCommandPool(m_device, m_commandPool, nullptr); // also frees its command buffer
}

} // namespace snake::gfx
