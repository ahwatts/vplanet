// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <sstream>
#include <vector>

#include "../vulkan.h"

#include "../Ocean.h"
#include "OceanPipeline.h"
#include "Pipeline.h"
#include "Renderer.h"
#include "Resource.h"
#include "System.h"

const Resource OCEAN_VERT_BYTECODE = LOAD_RESOURCE(ocean_vert_spv);
const Resource OCEAN_FRAG_BYTECODE = LOAD_RESOURCE(ocean_frag_spv);

gfx::OceanPipeline::OceanPipeline(Renderer *renderer)
    : Pipeline(renderer),
      m_vertex_shader{VK_NULL_HANDLE},
      m_fragment_shader{VK_NULL_HANDLE},
      m_num_indices{0},
      m_vertex_buffer{VK_NULL_HANDLE},
      m_index_buffer{VK_NULL_HANDLE},
      m_vertex_buffer_memory{VK_NULL_HANDLE},
      m_index_buffer_memory{VK_NULL_HANDLE}
{}

gfx::OceanPipeline::~OceanPipeline() {
    dispose();
}

void gfx::OceanPipeline::init() {
    initShaderModules();
    Pipeline::init();
}

void gfx::OceanPipeline::dispose() {
    cleanupGeometryBuffers();
    Pipeline::dispose();
    cleanupShaderModules();
}

void gfx::OceanPipeline::recordCommands(VkCommandBuffer cmd_buf, VkDescriptorSet xforms) {
    VkBuffer vertex_buffers[1] = { m_vertex_buffer };
    VkDeviceSize vertex_buffer_offsets[1] = { 0 };

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, vertex_buffers, vertex_buffer_offsets);
    vkCmdBindIndexBuffer(cmd_buf, m_index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &xforms, 0, nullptr);
    vkCmdDrawIndexed(cmd_buf, m_num_indices, 1, 0, 0, 0);
}

void gfx::OceanPipeline::setGeometry(const std::vector<OceanVertex> &verts, const std::vector<uint32_t> &indices) {
    System *gfx = m_renderer->system();
    cleanupGeometryBuffers();
    gfx->createBufferWithData(
        verts.data(), verts.size() * sizeof(OceanVertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        m_vertex_buffer, m_vertex_buffer_memory);
    gfx->createBufferWithData(
        indices.data(), indices.size() * sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        m_index_buffer, m_index_buffer_memory);
    m_num_indices = indices.size();
}

void gfx::OceanPipeline::cleanupGeometryBuffers() {
    VkDevice device = m_renderer->system()->device();
    m_num_indices = 0;
    if (device != VK_NULL_HANDLE) {
        if (m_vertex_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_vertex_buffer, nullptr);
            m_vertex_buffer = VK_NULL_HANDLE;
        }

        if (m_index_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, m_index_buffer, nullptr);
            m_index_buffer = VK_NULL_HANDLE;
        }

        if (m_vertex_buffer_memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_vertex_buffer_memory, nullptr);
            m_vertex_buffer_memory = VK_NULL_HANDLE;
        }

        if (m_index_buffer_memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_index_buffer_memory, nullptr);
            m_index_buffer_memory = VK_NULL_HANDLE;
        }
    }
}

void gfx::OceanPipeline::initShaderModules() {
    System *gfx = m_renderer->system();

    if (m_vertex_shader == VK_NULL_HANDLE) {
        gfx->createShaderModule(OCEAN_VERT_BYTECODE, m_vertex_shader);
    }

    if (m_fragment_shader == VK_NULL_HANDLE) {
        gfx->createShaderModule(OCEAN_FRAG_BYTECODE, m_fragment_shader);
    }
}

void gfx::OceanPipeline::cleanupShaderModules() {
    VkDevice device = m_renderer->system()->device();
    if (device != VK_NULL_HANDLE) {
        if (m_vertex_shader != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, m_vertex_shader, nullptr);
            m_vertex_shader = VK_NULL_HANDLE;
        }

        if (m_fragment_shader != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, m_fragment_shader, nullptr);
            m_fragment_shader = VK_NULL_HANDLE;
        }
    }
}

void gfx::OceanPipeline::initPipelineLayout() {
    if (m_pipeline_layout != VK_NULL_HANDLE) {
        return;
    }

    System *system = m_renderer->system();
    VkDevice device = system->device();
    VkDescriptorSetLayout xform_layout = system->transformUniforms().descriptorSetLayout();

    VkPipelineLayoutCreateInfo pl_ci;
    pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_ci.pNext = nullptr;
    pl_ci.flags = 0;
    pl_ci.setLayoutCount = 1;
    pl_ci.pSetLayouts = &xform_layout;
    pl_ci.pushConstantRangeCount = 0;
    pl_ci.pPushConstantRanges = nullptr;

    VkResult rslt = vkCreatePipelineLayout(device, &pl_ci, nullptr, &m_pipeline_layout);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create pipeline layout. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::OceanPipeline::initPipeline() {
    if (m_pipeline != VK_NULL_HANDLE) {
        return;
    }

    System *system = m_renderer->system();
    VkDevice device = system->device();
    VkExtent2D extent = system->swapchain().extent();
    VkRenderPass render_pass = m_renderer->renderPass();

    VkPipelineShaderStageCreateInfo ss_ci[2];
    ss_ci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss_ci[0].pNext = nullptr;
    ss_ci[0].flags = 0;
    ss_ci[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    ss_ci[0].module = m_vertex_shader;
    ss_ci[0].pName = "main";
    ss_ci[0].pSpecializationInfo = nullptr;

    ss_ci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ss_ci[1].pNext = nullptr;
    ss_ci[1].flags = 0;
    ss_ci[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    ss_ci[1].module = m_fragment_shader;
    ss_ci[1].pName = "main";
    ss_ci[1].pSpecializationInfo = nullptr;

    VkVertexInputBindingDescription bind_desc = OceanVertex::bindingDescription();
    std::array<VkVertexInputAttributeDescription, OceanVertex::NUM_ATTRIBUTES> attr_desc = OceanVertex::attributeDescription();

    VkPipelineVertexInputStateCreateInfo vert_in_ci;
    vert_in_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_in_ci.pNext = nullptr;
    vert_in_ci.flags = 0;
    vert_in_ci.vertexBindingDescriptionCount = 1;
    vert_in_ci.pVertexBindingDescriptions = &bind_desc;
    vert_in_ci.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr_desc.size());
    vert_in_ci.pVertexAttributeDescriptions = attr_desc.data();

    VkPipelineInputAssemblyStateCreateInfo input_asm_ci;
    input_asm_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm_ci.pNext = nullptr;
    input_asm_ci.flags = 0;
    input_asm_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_asm_ci.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo vp_ci;
    vp_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp_ci.pNext = nullptr;
    vp_ci.flags = 0;
    vp_ci.viewportCount = 1;
    vp_ci.pViewports = &viewport;
    vp_ci.scissorCount = 1;
    vp_ci.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo raster_ci;
    raster_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_ci.pNext = nullptr;
    raster_ci.flags = 0;
    raster_ci.depthClampEnable = VK_FALSE;
    raster_ci.rasterizerDiscardEnable = VK_FALSE;
    raster_ci.polygonMode = VK_POLYGON_MODE_FILL;
    raster_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    raster_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster_ci.depthBiasEnable = VK_FALSE;
    raster_ci.depthBiasConstantFactor = 0.0;
    raster_ci.depthBiasClamp = 0.0;
    raster_ci.depthBiasSlopeFactor = 0.0;
    raster_ci.lineWidth = 1.0;

    VkPipelineMultisampleStateCreateInfo msamp_ci;
    msamp_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msamp_ci.pNext = nullptr;
    msamp_ci.flags = 0;
    msamp_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msamp_ci.sampleShadingEnable = VK_FALSE;
    msamp_ci.minSampleShading = 0.0;
    msamp_ci.pSampleMask = nullptr;
    msamp_ci.alphaToCoverageEnable = VK_FALSE;
    msamp_ci.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_ci;
    depth_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_ci.pNext = nullptr;
    depth_ci.flags = 0;
    depth_ci.depthTestEnable = VK_TRUE;
    depth_ci.depthWriteEnable = VK_TRUE;
    depth_ci.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_ci.depthBoundsTestEnable = VK_FALSE;
    depth_ci.stencilTestEnable = VK_FALSE;
    depth_ci.front = {};
    depth_ci.back = {};
    depth_ci.minDepthBounds = 0.0;
    depth_ci.maxDepthBounds = 1.0;

    VkPipelineColorBlendAttachmentState blender;
    blender.blendEnable = VK_FALSE;
    blender.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blender.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blender.colorBlendOp = VK_BLEND_OP_ADD;
    blender.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blender.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blender.alphaBlendOp = VK_BLEND_OP_ADD;
    blender.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend_ci;
    blend_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_ci.pNext = nullptr;
    blend_ci.flags = 0;
    blend_ci.logicOpEnable = VK_FALSE;
    blend_ci.logicOp = VK_LOGIC_OP_COPY;
    blend_ci.attachmentCount = 1;
    blend_ci.pAttachments = &blender;
    blend_ci.blendConstants[0] = 0.0;
    blend_ci.blendConstants[1] = 0.0;
    blend_ci.blendConstants[2] = 0.0;
    blend_ci.blendConstants[3] = 0.0;

    // VkPipelineDynamicStateCreateInfo dyn_state_ci;
    // dyn_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // dyn_state_ci.pNext = nullptr;
    // dyn_state_ci.flags = 0;

    VkGraphicsPipelineCreateInfo pipeline_ci;
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.pNext = nullptr;
    pipeline_ci.flags = 0;
    pipeline_ci.stageCount = 2;
    pipeline_ci.pStages = ss_ci;
    pipeline_ci.pVertexInputState = &vert_in_ci;
    pipeline_ci.pInputAssemblyState = &input_asm_ci;
    pipeline_ci.pTessellationState = nullptr;
    pipeline_ci.pViewportState = &vp_ci;
    pipeline_ci.pRasterizationState = &raster_ci;
    pipeline_ci.pMultisampleState = &msamp_ci;
    pipeline_ci.pDepthStencilState = &depth_ci;
    pipeline_ci.pColorBlendState = &blend_ci;
    pipeline_ci.pDynamicState = nullptr;
    pipeline_ci.layout = m_pipeline_layout;
    pipeline_ci.renderPass = render_pass;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.basePipelineIndex = -1;

    VkResult rslt = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &m_pipeline);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create terrain renderer pipeline. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}
