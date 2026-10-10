#pragma once

#include "snake/core/types.hpp"

#include <vulkan/vulkan.h>

#include <span>

struct GLFWwindow;

namespace snake::platform {

enum class Key {
    Up,
    Down,
    Left,
    Right,
    Escape,
};

// A window with keyboard input. It also starts and stops GLFW, so create only one.
class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void close();

    void pollEvents(); // handle pending events without waiting
    void waitEvents(); // sleep until an event arrives

    bool isMinimized() const;
    bool isKeyDown(Key key) const;
    core::Size framebufferSize() const; // the drawable area, in pixels

    // True once after the drawable area changed size; reading it clears it.
    bool takeResized();

    // Seconds since the window was created.
    double time() const;

    // Vulkan needs OS-specific extensions and a surface to show images in a window.
    // GLFW knows what this OS needs, so the window provides them.
    std::span<const char* const> requiredVulkanExtensions() const;
    VkSurfaceKHR createVulkanSurface(VkInstance instance) const;

private:
    static void onFramebufferResized(GLFWwindow* handle, int width, int height);

    GLFWwindow* m_window = nullptr;
    bool m_resized = false;
};

} // namespace snake::platform
