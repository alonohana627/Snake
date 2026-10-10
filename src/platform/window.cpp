#include "snake/platform/window.hpp"

// Included after <vulkan/vulkan.h> (via window.hpp), so GLFW also declares
// its Vulkan helpers such as glfwCreateWindowSurface.
#include <GLFW/glfw3.h>

#include <cstdint>
#include <print>
#include <stdexcept>

namespace snake::platform {

namespace { // helpers

// GLFW calls this whenever one of its functions fails.
void _errorCallback(int code, const char* description)
{
    std::println(stderr, "GLFW error {}: {}", code, description);
}

int _toGlfwKey(Key key)
{
    switch (key) {
    case Key::Up:     return GLFW_KEY_UP;
    case Key::Down:   return GLFW_KEY_DOWN;
    case Key::Left:   return GLFW_KEY_LEFT;
    case Key::Right:  return GLFW_KEY_RIGHT;
    case Key::Escape: return GLFW_KEY_ESCAPE;
    }
    return GLFW_KEY_UNKNOWN;
}

} // namespace

Window::Window(int width, int height, const char* title)
{
    // The error callback is the one thing allowed before glfwInit,
    // so we also hear about errors during init.
    glfwSetErrorCallback(_errorCallback);
    if (glfwInit() == GLFW_FALSE) {
        throw std::runtime_error("glfwInit failed");
    }

    // By default GLFW also creates an OpenGL context; we draw with Vulkan, so ask for none.
    // The size is the inside (client area) - GLFW accounts for borders itself.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (m_window == nullptr) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    // Callbacks are plain functions with no way to reach our object, so store a
    // pointer to it on the GLFW window. (Safe: Window can't be copied or moved.)
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, onFramebufferResized);
}

Window::~Window()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(m_window) == GLFW_TRUE;
}

void Window::close()
{
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Window::pollEvents()
{
    glfwPollEvents();
}

void Window::waitEvents()
{
    glfwWaitEvents();
}

bool Window::isMinimized() const
{
    return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) == GLFW_TRUE;
}

bool Window::isKeyDown(Key key) const
{
    return glfwGetKey(m_window, _toGlfwKey(key)) == GLFW_PRESS;
}

core::Size Window::framebufferSize() const
{
    core::Size size;
    glfwGetFramebufferSize(m_window, &size.width, &size.height);
    return size;
}

bool Window::takeResized()
{
    const bool resized = m_resized;
    m_resized = false;
    return resized;
}

double Window::time() const
{
    return glfwGetTime();
}

std::span<const char* const> Window::requiredVulkanExtensions() const
{
    // On Windows: VK_KHR_surface + VK_KHR_win32_surface. GLFW owns the array.
    std::uint32_t count = 0;
    const char** names = glfwGetRequiredInstanceExtensions(&count);
    if (names == nullptr) {
        throw std::runtime_error("GLFW: Vulkan is not available on this system");
    }
    return {names, count};
}

VkSurfaceKHR Window::createVulkanSurface(VkInstance instance) const
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, m_window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("glfwCreateWindowSurface failed");
    }
    return surface;
}

// GLFW calls this when the drawable area changes size (from inside pollEvents/waitEvents).
void Window::onFramebufferResized(GLFWwindow* handle, int /*width*/, int /*height*/)
{
    auto* const window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    window->m_resized = true;
}

} // namespace snake::platform
