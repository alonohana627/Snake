#include "snake/gfx/check.hpp"

#include <vulkan/vk_enum_string_helper.h>

#include <format>
#include <stdexcept>

namespace snake::gfx {

void check(VkResult result, std::string_view what)
{
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::format("{} failed: {}", what, string_VkResult(result)));
    }
}

} // namespace snake::gfx
