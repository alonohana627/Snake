#include "snake/gfx/device.hpp"

#include "snake/gfx/check.hpp"
#include "snake/gfx/instance.hpp"
#include "snake/gfx/surface.hpp"

#include <algorithm>
#include <optional>
#include <print>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace snake::gfx {

namespace { // helpers

// A queue family that can both draw (graphics) and show images in our window (present).
std::optional<std::uint32_t> _findQueueFamily(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, families.data());

    for (std::uint32_t i = 0; i < count; ++i) {
        VkBool32 canPresent = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &canPresent);
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && canPresent == VK_TRUE) {
            return i;
        }
    }
    return std::nullopt;
}

// Does this GPU have everything we use? Vulkan 1.3, swapchains, dynamic rendering, synchronization2.
bool _isSuitable(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    if (properties.apiVersion < VK_API_VERSION_1_3) {
        return false;
    }

    std::uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensions.data());
    const bool hasSwapchain = std::ranges::any_of(extensions, [](const VkExtensionProperties& extension) {
        return std::string_view(extension.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    });
    if (!hasSwapchain) {
        return false;
    }

    // Features are queried through a pNext chain: each struct points to the next one.
    VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceFeatures2 features2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features13,
    };
    vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
    return features13.dynamicRendering == VK_TRUE && features13.synchronization2 == VK_TRUE;
}

} // namespace

Device::Device(const Instance& instance, const Surface& surface)
{
    // 1. Choose a GPU. Take the first suitable one, but prefer a discrete (separate) graphics card.
    std::uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance.handle(), &count, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(count);
    vkEnumeratePhysicalDevices(instance.handle(), &count, physicalDevices.data());

    for (VkPhysicalDevice physicalDevice : physicalDevices) {
        const std::optional<std::uint32_t> queueFamily = _findQueueFamily(physicalDevice, surface.handle());
        if (!queueFamily || !_isSuitable(physicalDevice)) {
            continue;
        }

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        const bool isDiscrete = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        if (m_physical == VK_NULL_HANDLE || isDiscrete) {
            m_physical = physicalDevice;
            m_queueFamily = *queueFamily;
        }
        if (isDiscrete) {
            break;
        }
    }

    if (m_physical == VK_NULL_HANDLE) {
        throw std::runtime_error("No GPU supports Vulkan 1.3 with dynamic rendering and synchronization2");
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_physical, &properties);
    std::println("GPU: {}", properties.deviceName);

    // 2. Open the logical device, with one queue from the family we found.
    const float priority = 1.0f;
    const VkDeviceQueueCreateInfo queueInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = m_queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };

    // Features must be explicitly enabled, or using them is an error.
    VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE, // simpler, clearer barriers and submits
        .dynamicRendering = VK_TRUE, // draw without VkRenderPass / VkFramebuffer objects
    };

    const char* const extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features13,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = extensions,
    };
    check(vkCreateDevice(m_physical, &createInfo, nullptr, &m_device), "vkCreateDevice");

    vkGetDeviceQueue(m_device, m_queueFamily, 0, &m_queue);
}

Device::~Device()
{
    vkDestroyDevice(m_device, nullptr);
}

void Device::waitIdle() const
{
    vkDeviceWaitIdle(m_device);
}

} // namespace snake::gfx
