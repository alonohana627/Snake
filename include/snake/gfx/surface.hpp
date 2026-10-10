#pragma once

#include <vulkan/vulkan.h>

namespace snake::platform {
class Window;
}

namespace snake::gfx {

class Instance;

// Vulkan's handle to the window we draw into.
class Surface {
public:
    Surface(const Instance& instance, const platform::Window& window);
    ~Surface();

    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    VkSurfaceKHR handle() const { return m_surface; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

} // namespace snake::gfx
