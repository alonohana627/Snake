#include "snake/gfx/rect_pipeline.hpp"

#include "snake/gfx/check.hpp"
#include "snake/gfx/device.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace snake::gfx {

namespace {

// SPIR-V compiled from src/gfx/shaders at build time (see src/CMakeLists.txt).
constexpr std::uint32_t kRectVertSpv[] = {
#include "rect.vert.spv.inc"
};
constexpr std::uint32_t kRectFragSpv[] = {
#include "rect.frag.spv.inc"
};

// Must match the push_constant block in the shaders, byte for byte.
// GLSL lays it out with std430 rules: vec4 is 16-byte aligned, vec2 is 8-byte aligned.
struct PushConstants {
    float color[4];
    float position[2];
    float size[2];
    float screenSize[2];
};
static_assert(offsetof(PushConstants, color) == 0);
static_assert(offsetof(PushConstants, position) == 16);
static_assert(offsetof(PushConstants, size) == 24);
static_assert(offsetof(PushConstants, screenSize) == 32);
static_assert(sizeof(PushConstants) == 40);

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

} // namespace

RectPipeline::RectPipeline(const Device& device, VkFormat colorFormat)
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

    const VkShaderModule vertexShader = _createShaderModule(m_device, kRectVertSpv);
    const VkShaderModule fragmentShader = _createShaderModule(m_device, kRectFragSpv);

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
    // No blending: the fragment's color simply replaces what's there.
    const VkPipelineColorBlendAttachmentState blendAttachment{
        .blendEnable = VK_FALSE,
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

RectPipeline::~RectPipeline()
{
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_layout, nullptr);
}

void RectPipeline::bind(VkCommandBuffer commandBuffer) const
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

void RectPipeline::draw(VkCommandBuffer commandBuffer, const core::Rect& rect, const core::Color& color,
                        VkExtent2D targetSize) const
{
    const PushConstants pushConstants{
        .color = {color.r, color.g, color.b, color.a},
        .position = {rect.x, rect.y},
        .size = {rect.width, rect.height},
        .screenSize = {static_cast<float>(targetSize.width), static_cast<float>(targetSize.height)},
    };
    vkCmdPushConstants(commandBuffer, m_layout, kPushConstantStages, 0, sizeof(pushConstants), &pushConstants);

    vkCmdDraw(commandBuffer, 4, 1, 0, 0); // 4 vertices, 1 instance
}

} // namespace snake::gfx
