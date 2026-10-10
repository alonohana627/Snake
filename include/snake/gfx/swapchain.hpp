#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace snake::gfx {

class Device;
class Surface;

// The set of images that take turns being shown in the window.
class Swapchain {
public:
    // windowSize: the window's drawable area in pixels, used when the surface lets us choose.
    Swapchain(const Device& device, const Surface& surface, VkExtent2D windowSize);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    // Rebuild for a new window size. Waits for the GPU to stop using the old images.
    void recreate(VkExtent2D windowSize);

    VkSwapchainKHR handle() const { return m_swapchain; }
    VkFormat format() const { return m_surfaceFormat.format; }
    VkExtent2D extent() const { return m_extent; }

    VkImage image(std::uint32_t index) const { return m_images[index]; }
    VkImageView imageView(std::uint32_t index) const { return m_imageViews[index]; }

    // Signaled when rendering into image `index` is done; presenting waits on it.
    // One per image (not per frame): a present may still be waiting on its
    // semaphore when the next frame starts.
    VkSemaphore renderFinished(std::uint32_t index) const { return m_renderFinished[index]; }

private:
    void create(VkExtent2D windowSize, VkSwapchainKHR oldSwapchain);
    void destroyImageResources();

    const Device& m_device;
    const Surface& m_surface;

    VkSurfaceFormatKHR m_surfaceFormat{};
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkExtent2D m_extent{};
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    std::vector<VkSemaphore> m_renderFinished;
};

} // namespace snake::gfx
