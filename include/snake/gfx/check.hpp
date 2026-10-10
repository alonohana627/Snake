#pragma once

#include <vulkan/vulkan.h>

#include <string_view>

namespace snake::gfx {

// Most Vulkan calls return a VkResult. Throws std::runtime_error unless it's VK_SUCCESS.
void check(VkResult result, std::string_view what);

} // namespace snake::gfx
