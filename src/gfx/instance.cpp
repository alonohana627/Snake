#include "snake/gfx/instance.hpp"

#include "snake/gfx/check.hpp"

#include <algorithm>
#include <cstdint>
#include <print>
#include <string_view>
#include <vector>

namespace snake::gfx {

namespace {

#ifdef NDEBUG
constexpr bool kWantValidation = false;
#else
constexpr bool kWantValidation = true;
#endif
constexpr const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";

} // namespace

namespace { // helpers

// The validation layer calls this with its messages.
VKAPI_ATTR VkBool32 VKAPI_CALL _debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*severity*/,
    VkDebugUtilsMessageTypeFlagsEXT /*types*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*userData*/)
{
    std::println(stderr, "[Vulkan] {}", data->pMessage);
    return VK_FALSE; // VK_FALSE = don't abort the call that caused the message
}

VkDebugUtilsMessengerCreateInfoEXT _debugMessengerInfo()
{
    return {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = _debugCallback,
    };
}

bool _hasValidationLayer()
{
    std::uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());

    return std::ranges::any_of(layers, [](const VkLayerProperties& layer) {
        return std::string_view(layer.layerName) == kValidationLayer;
    });
}

} // namespace

Instance::Instance(std::span<const char* const> windowExtensions)
{
    std::vector<const char*> extensions(windowExtensions.begin(), windowExtensions.end());
    std::vector<const char*> layers;

    bool validation = false;
    if (kWantValidation) {
        if (_hasValidationLayer()) {
            layers.push_back(kValidationLayer);
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            validation = true;
        } else {
            std::println(stderr, "Validation layer not found - running without it. "
                                 "Set VK_ADD_LAYER_PATH to the folder with VkLayer_khronos_validation.json.");
        }
    }

    const VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Snake",
        .apiVersion = VK_API_VERSION_1_3,
    };
    const VkDebugUtilsMessengerCreateInfoEXT debugInfo = _debugMessengerInfo();
    const VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        // Chaining the messenger settings here also reports problems
        // inside vkCreateInstance / vkDestroyInstance themselves.
        .pNext = validation ? &debugInfo : nullptr,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    check(vkCreateInstance(&createInfo, nullptr, &m_instance), "vkCreateInstance");

    if (validation) {
        // Extension functions aren't exported by the Vulkan loader; we have to look them up.
        const auto createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
        // Not fatal (and not thrown: the destructor wouldn't run and the instance would leak).
        if (createMessenger == nullptr
            || createMessenger(m_instance, &debugInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
            std::println(stderr, "vkCreateDebugUtilsMessengerEXT failed - validation messages may be missing.");
        }
    }
}

Instance::~Instance()
{
    if (m_debugMessenger != VK_NULL_HANDLE) {
        const auto destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
        destroyMessenger(m_instance, m_debugMessenger, nullptr);
    }
    vkDestroyInstance(m_instance, nullptr);
}

} // namespace snake::gfx
