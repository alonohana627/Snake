#include "snake/gfx/surface.hpp"

#include "snake/gfx/instance.hpp"
#include "snake/platform/window.hpp"

namespace snake::gfx {

// Creating a surface is OS-specific, so the window does it for us.
Surface::Surface(const Instance& instance, const platform::Window& window)
    : m_instance(instance.handle())
    , m_surface(window.createVulkanSurface(instance.handle()))
{
}

Surface::~Surface()
{
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

} // namespace snake::gfx
