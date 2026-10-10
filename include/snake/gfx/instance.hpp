#pragma once

#include <vulkan/vulkan.h>

#include <span>

namespace snake::gfx {

// Our connection to Vulkan. In debug builds it also turns on the validation
// layer, which checks every Vulkan call and prints problems to stderr.
class Instance {
public:
    // windowExtensions: what the OS needs to show Vulkan images in a window.
    explicit Instance(std::span<const char* const> windowExtensions);
    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    VkInstance handle() const { return m_instance; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
};

} // namespace snake::gfx
