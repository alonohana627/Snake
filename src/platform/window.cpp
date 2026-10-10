#include "snake/platform/window.hpp"

// Included after <vulkan/vulkan.h> (via window.hpp), so GLFW also declares
// its Vulkan helpers such as glfwCreateWindowSurface.
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <print>
#include <stdexcept>
#include <utility>

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
    case Key::Enter:  return GLFW_KEY_ENTER;
    case Key::F11:    return GLFW_KEY_F11;
    }
    return GLFW_KEY_UNKNOWN;
}

// The reverse of _toGlfwKey. Keys we don't use give std::nullopt.
std::optional<Key> _fromGlfwKey(int glfwKey)
{
    switch (glfwKey) {
    case GLFW_KEY_UP:       return Key::Up;
    case GLFW_KEY_DOWN:     return Key::Down;
    case GLFW_KEY_LEFT:     return Key::Left;
    case GLFW_KEY_RIGHT:    return Key::Right;
    case GLFW_KEY_ESCAPE:   return Key::Escape;
    case GLFW_KEY_ENTER:    return Key::Enter;
    case GLFW_KEY_KP_ENTER: return Key::Enter; // the numpad one counts too
    case GLFW_KEY_F11:      return Key::F11;
    default:                return std::nullopt;
    }
}

// The monitor showing the biggest part of the window (the primary one if none does).
// That's the one to go fullscreen on - the window may be on a second monitor.
GLFWmonitor* _monitorUnder(GLFWwindow* window)
{
    int windowX = 0;
    int windowY = 0;
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowPos(window, &windowX, &windowY);
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    int count = 0;
    GLFWmonitor** const monitors = glfwGetMonitors(&count);

    GLFWmonitor* best = glfwGetPrimaryMonitor();
    int bestArea = 0;
    for (GLFWmonitor* const monitor : std::span(monitors, static_cast<std::size_t>(count))) {
        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        const GLFWvidmode* const mode = glfwGetVideoMode(monitor);

        // How much the two rectangles overlap (negative = they don't).
        const int overlapWidth = std::min(windowX + windowWidth, monitorX + mode->width) - std::max(windowX, monitorX);
        const int overlapHeight = std::min(windowY + windowHeight, monitorY + mode->height) - std::max(windowY, monitorY);
        const int area = std::max(overlapWidth, 0) * std::max(overlapHeight, 0);
        if (area > bestArea) {
            best = monitor;
            bestArea = area;
        }
    }
    return best;
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
    glfwSetKeyCallback(m_window, onKey);

    // Dragging the window's edges keeps width:height as created.
    glfwSetWindowAspectRatio(m_window, width, height);
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

void Window::setTitle(const std::string& title)
{
    glfwSetWindowTitle(m_window, title.c_str());
}

void Window::pollEvents()
{
    glfwPollEvents();
}

void Window::waitEvents()
{
    glfwWaitEvents();
}

void Window::setFullscreen(bool fullscreen)
{
    if (fullscreen == m_fullscreen) {
        return;
    }

    if (fullscreen) {
        // Remember the windowed position and size, to restore them later.
        ScreenRect& rect = m_windowedRect;
        glfwGetWindowPos(m_window, &rect.x, &rect.y);
        glfwGetWindowSize(m_window, &rect.width, &rect.height);

        GLFWmonitor* const monitor = _monitorUnder(m_window);
        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        const GLFWvidmode* const mode = glfwGetVideoMode(monitor);

        // No border, exactly covering the monitor. The monitor stays nullptr
        // ("not a real fullscreen window"): that's what makes it borderless.
        glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(m_window, nullptr, monitorX, monitorY, mode->width, mode->height, GLFW_DONT_CARE);
    } else {
        const ScreenRect& rect = m_windowedRect;
        glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(m_window, nullptr, rect.x, rect.y, rect.width, rect.height, GLFW_DONT_CARE);
    }
    m_fullscreen = fullscreen;
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

std::vector<Key> Window::takePressedKeys()
{
    return std::exchange(m_pressedKeys, {});
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

// GLFW calls this for every key event (from inside pollEvents/waitEvents).
void Window::onKey(GLFWwindow* handle, int key, int /*scancode*/, int action, int /*mods*/)
{
    // action is GLFW_PRESS, GLFW_REPEAT (key held down) or GLFW_RELEASE.
    // Only the moment a key goes down counts as a press.
    if (action != GLFW_PRESS) {
        return;
    }
    if (const std::optional<Key> pressed = _fromGlfwKey(key)) {
        auto* const window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
        window->m_pressedKeys.push_back(*pressed);
    }
}

} // namespace snake::platform
