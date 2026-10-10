#include "snake/gfx/swapchain.hpp"

#include "snake/gfx/check.hpp"
#include "snake/gfx/device.hpp"
#include "snake/gfx/surface.hpp"

#include <algorithm>
#include <limits>

namespace snake::gfx {

namespace { // helpers

// Prefer 8-bit BGRA/RGBA with sRGB encoding: we work in linear colors and the GPU
// converts to sRGB when writing, which is what the monitor expects.
VkSurfaceFormatKHR _chooseSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    std::uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, formats.data());

    for (const VkSurfaceFormatKHR& format : formats) {
        const bool isSrgb = format.format == VK_FORMAT_B8G8R8A8_SRGB || format.format == VK_FORMAT_R8G8B8A8_SRGB;
        if (isSrgb && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats.at(0);
}

} // namespace

Swapchain::Swapchain(const Device& device, const Surface& surface, VkExtent2D windowSize)
    : m_device(device)
    , m_surface(surface)
    , m_surfaceFormat(_chooseSurfaceFormat(device.physical(), surface.handle()))
{
    // If create() throws halfway, the destructor won't run (the object never
    // finished constructing), so clean up what was already made ourselves.
    try {
        create(windowSize, VK_NULL_HANDLE);
    } catch (...) {
        destroyImageResources();
        vkDestroySwapchainKHR(m_device.handle(), m_swapchain, nullptr);
        throw;
    }
}

Swapchain::~Swapchain()
{
    destroyImageResources();
    vkDestroySwapchainKHR(m_device.handle(), m_swapchain, nullptr);
}

void Swapchain::recreate(VkExtent2D windowSize)
{
    // The GPU may still be using the old images. Simple and safe: wait for it to finish.
    m_device.waitIdle();

    const VkSwapchainKHR oldSwapchain = m_swapchain;
    destroyImageResources();
    create(windowSize, oldSwapchain); // passing the old one lets the driver reuse resources
    vkDestroySwapchainKHR(m_device.handle(), oldSwapchain, nullptr);
}

void Swapchain::create(VkExtent2D windowSize, VkSwapchainKHR oldSwapchain)
{
    const VkDevice device = m_device.handle();

    VkSurfaceCapabilitiesKHR capabilities{};
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device.physical(), m_surface.handle(), &capabilities),
          "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    // Usually the surface dictates the size. 0xFFFFFFFF means "you choose":
    // then use the window's size, within the allowed range.
    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == std::numeric_limits<std::uint32_t>::max()) {
        extent.width = std::clamp(windowSize.width,
                                  capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(windowSize.height,
                                   capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    // One more than the minimum, so we rarely wait on the driver. maxImageCount 0 means "no limit".
    std::uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    const VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface.handle(),
        .minImageCount = imageCount,
        .imageFormat = m_surfaceFormat.format,
        .imageColorSpace = m_surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // we render into these images
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,     // used by one queue family only
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR, // vsync; the one mode every driver supports
        .clipped = VK_TRUE,
        .oldSwapchain = oldSwapchain,
    };
    // Create into a local first: if this fails, m_swapchain still holds the old
    // swapchain (when recreating), and the destructor cleans that one up.
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    check(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain), "vkCreateSwapchainKHR");
    m_swapchain = swapchain;
    m_extent = extent;

    // The swapchain created the images; we just get their handles.
    vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, m_images.data());

    for (VkImage image : m_images) {
        // An image view says how to look at an image (format, which part). Rendering needs one.
        const VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_surfaceFormat.format,
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };
        VkImageView view = VK_NULL_HANDLE;
        check(vkCreateImageView(device, &viewInfo, nullptr, &view), "vkCreateImageView");
        m_imageViews.push_back(view);

        const VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphore semaphore = VK_NULL_HANDLE;
        check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore), "vkCreateSemaphore");
        m_renderFinished.push_back(semaphore);
    }
}

void Swapchain::destroyImageResources()
{
    for (VkSemaphore semaphore : m_renderFinished) {
        vkDestroySemaphore(m_device.handle(), semaphore, nullptr);
    }
    for (VkImageView view : m_imageViews) {
        vkDestroyImageView(m_device.handle(), view, nullptr);
    }
    m_renderFinished.clear();
    m_imageViews.clear();
    m_images.clear(); // owned by the swapchain itself, just forget the handles
}

} // namespace snake::gfx
