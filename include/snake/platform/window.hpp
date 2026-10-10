#pragma once

#include "snake/core/types.hpp"

#include <vulkan/vulkan.h>

#include <span>
#include <string>
#include <vector>

struct GLFWwindow;

namespace snake::platform {

enum class Key {
    Up,
    Down,
    Left,
    Right,
    Escape,
    Enter,
    F11,
};

// A window with keyboard input. It also starts and stops GLFW, so create only one.
class Window {
public:
    // width x height is the drawable area. Resizing by hand keeps that shape
    // (e.g. 1920x1080 stays 16:9).
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void close();

    void setTitle(const std::string& title);

    void pollEvents(); // handle pending events without waiting
    void waitEvents(); // sleep until an event arrives

    // Fullscreen is "borderless": the window loses its border and covers the whole
    // monitor it's on. No video mode change, so switching is instant and Alt+Tab works.
    // Turning it off puts the window back where it was, at its old size.
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const { return m_fullscreen; }

    bool isMinimized() const;
    bool isKeyDown(Key key) const;

    // Keys pressed since the last call, in the order they were pressed.
    // Holding a key down counts as one press. Reading them clears the list.
    std::vector<Key> takePressedKeys();
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
    static void onKey(GLFWwindow* handle, int key, int scancode, int action, int mods);

    // A rectangle in screen coordinates (where windows and monitors are on the desktop).
    struct ScreenRect {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    GLFWwindow* m_window = nullptr;
    bool m_resized = false;
    bool m_fullscreen = false;
    ScreenRect m_windowedRect; // where the window was before going fullscreen
    std::vector<Key> m_pressedKeys;
};

} // namespace snake::platform
