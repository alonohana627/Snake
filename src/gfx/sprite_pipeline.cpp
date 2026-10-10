#include "snake/gfx/sprite_pipeline.hpp"

#include "snake/gfx/check.hpp"
#include "snake/gfx/device.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace snake::gfx {

namespace {

// SPIR-V compiled from src/gfx/shaders at build time (see src/gfx/CMakeLists.txt).
constexpr std::uint32_t kSpriteVertSpv[] = {
#include "sprite.vert.spv.inc"
};
constexpr std::uint32_t kRectFragSpv[] = {
#include "rect.frag.spv.inc"
};
constexpr std::uint32_t kSnakeHeadFragSpv[] = {
#include "snake_head.frag.spv.inc"
};
constexpr std::uint32_t kFoodFragSpv[] = {
#include "food.frag.spv.inc"
};
constexpr std::uint32_t kBackgroundFragSpv[] = {
#include "background.frag.spv.inc"
};
constexpr std::uint32_t kGlyphFragSpv[] = {
#include "glyph.frag.spv.inc"
};

// Must match the push_constant block in the shaders, byte for byte.
// GLSL lays it out with std430 rules: vec4 is 16-byte aligned, vec2 and uvec2 are
// 8-byte aligned. So `glyph` starts at 48, not 44: GLSL skips 4 bytes, and alignas(8)
// makes C++ skip the same 4.
struct PushConstants {
    float color[4];
    float position[2];
    float size[2];
    float screenSize[2];
    std::int32_t quarterTurns;
    alignas(8) std::uint32_t glyph[2]; // GLSL has no 64-bit ints by default: low 32 bits, high 32 bits
};
static_assert(offsetof(PushConstants, color) == 0);
static_assert(offsetof(PushConstants, position) == 16);
static_assert(offsetof(PushConstants, size) == 24);
static_assert(offsetof(PushConstants, screenSize) == 32);
static_assert(offsetof(PushConstants, quarterTurns) == 40);
static_assert(offsetof(PushConstants, glyph) == 48);
static_assert(sizeof(PushConstants) == 56);

constexpr VkShaderStageFlags kPushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

} // namespace

namespace { // helpers

VkShaderModule _createShaderModule(VkDevice device, std::span<const std::uint32_t> spirv)
{
    const VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv.size_bytes(),
        .pCode = spirv.data(),
    };
    VkShaderModule module = VK_NULL_HANDLE;
    check(vkCreateShaderModule(device, &createInfo, nullptr, &module), "vkCreateShaderModule");
    return module;
}

// Each shape is drawn by its own fragment shader.
std::span<const std::uint32_t> _fragmentShader(core::Shape shape)
{
    switch (shape) {
    case core::Shape::Rect:       return kRectFragSpv;
    case core::Shape::SnakeHead:  return kSnakeHeadFragSpv;
    case core::Shape::Food:       return kFoodFragSpv;
    case core::Shape::Background: return kBackgroundFragSpv;
    case core::Shape::Glyph:      return kGlyphFragSpv;
    }
    return kRectFragSpv;
}

} // namespace

SpritePipeline::SpritePipeline(const Device& device, VkFormat colorFormat, core::Shape shape)
    : m_device(device.handle())
{
    // The layout says what data the shaders get from outside: here, only push constants.
    const VkPushConstantRange pushConstantRange{
        .stageFlags = kPushConstantStages,
        .offset = 0,
        .size = sizeof(PushConstants),
    };
    const VkPipelineLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };
    check(vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_layout), "vkCreatePipelineLayout");

    const VkShaderModule vertexShader = _createShaderModule(m_device, kSpriteVertSpv);
    const VkShaderModule fragmentShader = _createShaderModule(m_device, _fragmentShader(shape));

    const VkPipelineShaderStageCreateInfo stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShader,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShader,
            .pName = "main",
        },
    };

    // No vertex buffers: the vertex shader builds positions from gl_VertexIndex.
    const VkPipelineVertexInputStateCreateInfo vertexInput{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };
    // 4 vertices as a strip = 2 triangles sharing an edge = our rectangle.
    const VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    };
    // Viewport and scissor are set per frame (dynamic state), only their count is fixed.
    const VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };
    const VkPipelineRasterizationStateCreateInfo rasterization{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE, // draw triangles whichever way they face
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };
    const VkPipelineMultisampleStateCreateInfo multisample{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    // Alpha blending: new color * alpha + what's already there * (1 - alpha).
    // Shapes use alpha for their outline (and its soft, anti-aliased edge);
    // fully opaque pixels (alpha 1) simply replace what's there.
    const VkPipelineColorBlendAttachmentState blendAttachment{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    const VkPipelineColorBlendStateCreateInfo colorBlend{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blendAttachment,
    };
    const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    const VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
    };
    // Dynamic rendering: instead of a VkRenderPass, just tell the pipeline the image format it draws into.
    const VkPipelineRenderingCreateInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
    };

    const VkGraphicsPipelineCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingInfo,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisample,
        .pColorBlendState = &colorBlend,
        .pDynamicState = &dynamicState,
        .layout = m_layout,
    };
    const VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline);

    // Shader modules are only needed while creating the pipeline.
    vkDestroyShaderModule(m_device, fragmentShader, nullptr);
    vkDestroyShaderModule(m_device, vertexShader, nullptr);

    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(m_device, m_layout, nullptr); // the destructor won't run
        check(result, "vkCreateGraphicsPipelines");
    }
}

SpritePipeline::~SpritePipeline()
{
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_layout, nullptr);
}

void SpritePipeline::bind(VkCommandBuffer commandBuffer) const
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

void SpritePipeline::draw(VkCommandBuffer commandBuffer, const core::Sprite& sprite, core::Size canvas) const
{
    const core::Rect& rect = sprite.rect;
    const core::Color& color = sprite.color;
    const PushConstants pushConstants{
        .color = {color.r, color.g, color.b, color.a},
        .position = {rect.x, rect.y},
        .size = {rect.width, rect.height},
        .screenSize = {static_cast<float>(canvas.width), static_cast<float>(canvas.height)},
        .quarterTurns = sprite.quarterTurns,
        .glyph = {static_cast<std::uint32_t>(sprite.glyph), static_cast<std::uint32_t>(sprite.glyph >> 32)},
    };
    vkCmdPushConstants(commandBuffer, m_layout, kPushConstantStages, 0, sizeof(pushConstants), &pushConstants);

    vkCmdDraw(commandBuffer, 4, 1, 0, 0); // 4 vertices, 1 instance
}

} // namespace snake::gfx
