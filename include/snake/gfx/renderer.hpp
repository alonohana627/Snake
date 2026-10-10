#pragma once

#include "snake/core/types.hpp"
#include "snake/gfx/device.hpp"
#include "snake/gfx/frame.hpp"
#include "snake/gfx/instance.hpp"
#include "snake/gfx/sprite_pipeline.hpp"
#include "snake/gfx/surface.hpp"
#include "snake/gfx/swapchain.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace snake::platform {
class Window;
}

namespace snake::gfx {

// Draws frames into a window. Owns all the Vulkan pieces and ties them together.
class Renderer {
public:
    explicit Renderer(platform::Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Draws one frame: a clear background, then the sprites in order (later ones on top).
    void drawFrame(std::span<const core::Sprite> sprites);

private:
    // How many frames the CPU may prepare while the GPU is still busy with earlier ones.
    static constexpr std::size_t kFramesInFlight = 2;

    void recreateSwapchain();
    void recordCommands(VkCommandBuffer commandBuffer, std::uint32_t imageIndex,
                        std::span<const core::Sprite> sprites) const;
    const SpritePipeline& pipelineFor(core::Shape shape) const;
    void submit(const Frame& frame, std::uint32_t imageIndex) const;
    void present(std::uint32_t imageIndex);

    platform::Window& m_window;

    // Members are created top to bottom and destroyed bottom to top - exactly the
    // order Vulkan needs (e.g. the device must outlive everything made from it).
    Instance m_instance;
    Surface m_surface;
    Device m_device;
    Swapchain m_swapchain;
    SpritePipeline m_rectPipeline;      // core::Shape::Rect
    SpritePipeline m_snakeHeadPipeline; // core::Shape::SnakeHead
    std::array<Frame, kFramesInFlight> m_frames;

    std::uint64_t m_frameNumber = 0;
};

} // namespace snake::gfx
