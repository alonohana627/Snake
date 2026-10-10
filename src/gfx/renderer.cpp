#include "snake/gfx/renderer.hpp"

#include "snake/gfx/check.hpp"
#include "snake/platform/window.hpp"

namespace snake::gfx {

namespace {

constexpr VkClearColorValue kBackgroundColor{.float32 = {0.05f, 0.05f, 0.07f, 1.0f}};

} // namespace

namespace { // helpers

VkExtent2D _toExtent(core::Size size)
{
    return {static_cast<std::uint32_t>(size.width), static_cast<std::uint32_t>(size.height)};
}

// Records an image layout transition plus the synchronization around it:
// work in srcStage (with srcAccess) must finish before dstStage (with dstAccess) starts.
void _transitionImage(VkCommandBuffer commandBuffer, VkImage image,
                      VkImageLayout oldLayout, VkImageLayout newLayout,
                      VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess,
                      VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess)
{
    const VkImageMemoryBarrier2 barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = srcStage,
        .srcAccessMask = srcAccess,
        .dstStageMask = dstStage,
        .dstAccessMask = dstAccess,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    const VkDependencyInfo dependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };
    vkCmdPipelineBarrier2(commandBuffer, &dependency);
}

} // namespace

Renderer::Renderer(platform::Window& window)
    : m_window(window)
    , m_instance(window.requiredVulkanExtensions())
    , m_surface(m_instance, window)
    , m_device(m_instance, m_surface)
    , m_swapchain(m_device, m_surface, _toExtent(window.framebufferSize()))
    , m_rectPipeline(m_device, m_swapchain.format())
    , m_frames{Frame(m_device), Frame(m_device)} // one per frame in flight
{
}

Renderer::~Renderer()
{
    // The members destroy themselves (in reverse order) after this,
    // but the GPU must not be using any of them anymore.
    m_device.waitIdle();
}

void Renderer::drawFrame(std::span<const core::ColoredRect> rects)
{
    if (m_window.takeResized()) {
        recreateSwapchain();
    }

    const Frame& frame = m_frames[m_frameNumber % kFramesInFlight];

    // 1. Wait until the GPU is done with the last frame that used this slot,
    //    so its command buffer and semaphore can be reused.
    frame.waitUntilFree();

    // 2. Ask for the next swapchain image. imageAcquired gets signaled once
    //    the image is really free (the screen may still be showing it).
    std::uint32_t imageIndex = 0;
    const VkResult result = vkAcquireNextImageKHR(m_device.handle(), m_swapchain.handle(), UINT64_MAX,
                                                  frame.imageAcquired(), VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    if (result != VK_SUBOPTIMAL_KHR) { // suboptimal still works; we rebuild after presenting
        check(result, "vkAcquireNextImageKHR");
    }

    // Reset only now that we know we'll submit work that signals the fence again.
    frame.reset();

    // 3. Record, 4. submit, 5. present.
    recordCommands(frame.commandBuffer(), imageIndex, rects);
    submit(frame, imageIndex);
    present(imageIndex);

    ++m_frameNumber;
}

void Renderer::recreateSwapchain()
{
    // Minimized: the window is 0x0 and no swapchain can be made. Nothing to draw anyway.
    const core::Size size = m_window.framebufferSize();
    if (size.width == 0 || size.height == 0) {
        return;
    }
    m_swapchain.recreate(_toExtent(size));
}

void Renderer::recordCommands(VkCommandBuffer commandBuffer, std::uint32_t imageIndex,
                              std::span<const core::ColoredRect> rects) const
{
    const VkImage image = m_swapchain.image(imageIndex);
    const VkExtent2D extent = m_swapchain.extent();

    const VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer");

    // Get the image ready to be drawn into. UNDEFINED = we don't care about its
    // old contents (we clear it anyway). Waits on the same stage the acquire
    // semaphore waits on, so the transition happens after the image is free.
    _transitionImage(commandBuffer, image,
                     VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE,
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

    const VkRenderingAttachmentInfo colorAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_swapchain.imageView(imageIndex),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,   // start by filling with clearValue
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // keep the result (we want to show it)
        .clearValue = {.color = kBackgroundColor},
    };
    const VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
    };
    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    const VkViewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    const VkRect2D scissor{{0, 0}, extent};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Bind once, then one draw per rectangle (each with its own push constants).
    m_rectPipeline.bind(commandBuffer);
    for (const core::ColoredRect& coloredRect : rects) {
        m_rectPipeline.draw(commandBuffer, coloredRect.rect, coloredRect.color, extent);
    }

    vkCmdEndRendering(commandBuffer);

    // Get the image ready to be shown. Nothing after this in the command buffer
    // uses it; the renderFinished semaphore makes the writes visible to present.
    _transitionImage(commandBuffer, image,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE);

    check(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");
}

// Wait for the image to be free before writing colors, signal renderFinished
// when done, and signal the frame's fence so the CPU knows too.
void Renderer::submit(const Frame& frame, std::uint32_t imageIndex) const
{
    const VkSemaphoreSubmitInfo waitInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = frame.imageAcquired(),
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    const VkSemaphoreSubmitInfo signalInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = m_swapchain.renderFinished(imageIndex),
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    };
    const VkCommandBufferSubmitInfo commandBufferInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = frame.commandBuffer(),
    };
    const VkSubmitInfo2 submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalInfo,
    };
    check(vkQueueSubmit2(m_device.queue(), 1, &submitInfo, frame.inFlight()), "vkQueueSubmit2");
}

// Show the image once rendering finished.
void Renderer::present(std::uint32_t imageIndex)
{
    const VkSemaphore renderFinished = m_swapchain.renderFinished(imageIndex);
    const VkSwapchainKHR swapchain = m_swapchain.handle();
    const VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinished,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
    };
    const VkResult result = vkQueuePresentKHR(m_device.queue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else {
        check(result, "vkQueuePresentKHR");
    }
}

} // namespace snake::gfx
