// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <array>
#include <cstring>
#include <sstream>
#include <vector>
#include "../vulkan.h"
#include "../Terrain.h"
#include "DepthBuffer.h"
#include "System.h"
#include "TerrainRenderer.h"

const Resource TERRAIN_VERT_BYTECODE = LOAD_RESOURCE(terrain_vert_spv);
const Resource TERRAIN_FRAG_BYTECODE = LOAD_RESOURCE(terrain_frag_spv);

gfx::TerrainRenderer::TerrainRenderer(System *system)
    : m_system{system},
      m_vertex_shader{VK_NULL_HANDLE},
      m_fragment_shader{VK_NULL_HANDLE},
      m_render_pass{VK_NULL_HANDLE},
      m_pipeline_layout{VK_NULL_HANDLE},
      m_pipeline{VK_NULL_HANDLE},
      m_framebuffers{},
      m_num_indices{0},
      m_vertex_buffer{VK_NULL_HANDLE},
      m_index_buffer{VK_NULL_HANDLE},
      m_vertex_buffer_memory{VK_NULL_HANDLE},
      m_index_buffer_memory{VK_NULL_HANDLE}
{}

gfx::TerrainRenderer::~TerrainRenderer() {
    dispose();
}

void gfx::TerrainRenderer::init(const std::vector<VkImageView> &color_buffers, const gfx::DepthBuffer &depth_buffer) {
    if (m_system == nullptr || m_system->device() == VK_NULL_HANDLE) {
        std::stringstream msg;
        msg << "Cannot initialize terrain renderer before system";
        throw std::runtime_error(msg.str());
    }

    initShaderModules();
    initRenderPass();
    initPipelineLayout();
    initPipeline();
    initFramebuffers(color_buffers, depth_buffer);
}

void gfx::TerrainRenderer::dispose() {
    cleanupGeometryBuffers();
    cleanupFramebuffers();
    cleanupPipeline();
    cleanupShaderModules();
    cleanupRenderPass();
    cleanupPipelineLayout();
}

VkRenderPass gfx::TerrainRenderer::renderPass() const {
    return m_render_pass;
}

void gfx::TerrainRenderer::setGeometry(const std::vector<TerrainVertex> &verts, const std::vector<uint32_t> &indices) {
    cleanupGeometryBuffers();
    initGeometryBuffer(m_vertex_buffer, m_vertex_buffer_memory, verts, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    initGeometryBuffer(m_index_buffer, m_index_buffer_memory, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    m_num_indices = indices.size();
}

void gfx::TerrainRenderer::recordCommands(const VkCommandBuffer &cmd_buf, const VkDescriptorSet &xforms, uint32_t framebuffer_index)
{
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clear_values[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rp_bi;
    rp_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_bi.pNext = nullptr;
    rp_bi.renderPass = m_render_pass;
    rp_bi.framebuffer = m_framebuffers[framebuffer_index];
    rp_bi.renderArea.offset = { 0, 0 };
    rp_bi.renderArea.extent = m_system->swapchain().extent();
    rp_bi.clearValueCount = clear_values.size();
    rp_bi.pClearValues = clear_values.data();

    VkBuffer vertex_buffers[1] = { m_vertex_buffer };
    VkDeviceSize vertex_buffer_offsets[1] = { 0 };

    vkCmdBeginRenderPass(cmd_buf, &rp_bi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, vertex_buffers, vertex_buffer_offsets);
    vkCmdBindIndexBuffer(cmd_buf, m_index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &xforms, 0, nullptr);
    vkCmdDrawIndexed(cmd_buf, m_num_indices, 1, 0, 0, 0);
    vkCmdEndRenderPass(cmd_buf);
}

void gfx::TerrainRenderer::initShaderModules() {
    if (m_vertex_shader == VK_NULL_HANDLE) {
        m_vertex_shader = createShaderModule(TERRAIN_VERT_BYTECODE);
    }

    if (m_fragment_shader == VK_NULL_HANDLE) {
        m_fragment_shader = createShaderModule(TERRAIN_FRAG_BYTECODE);
    }
}

void gfx::TerrainRenderer::cleanupShaderModules() {
    VkDevice device = m_system->device();
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

VkShaderModule gfx::TerrainRenderer::createShaderModule(const Resource &rsrc) {
    VkDevice device = m_system->device();

    VkShaderModuleCreateInfo sm_ci;
    sm_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm_ci.pNext = nullptr;
    sm_ci.flags = 0;
    sm_ci.codeSize = rsrc.size();
    sm_ci.pCode = reinterpret_cast<const uint32_t*>(rsrc.data());

    VkShaderModule module{VK_NULL_HANDLE};
    VkResult rslt = vkCreateShaderModule(device, &sm_ci, nullptr, &module);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create shader module. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    return module;
}

void gfx::TerrainRenderer::initRenderPass() {
    if (m_render_pass != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();
    const Swapchain &swapchain = m_system->swapchain();
    const DepthBuffer &depth_buffer = m_system->depthBuffer();
    VkFormat color_format = swapchain.format().format;
    VkFormat depth_format = depth_buffer.format();

    VkAttachmentDescription attachments[2];

    attachments[0].flags = 0;
    attachments[0].format = color_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].flags = 0;
    attachments[1].format = depth_format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_ref;
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_ref;
    depth_ref.attachment = 1;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass;
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depth_ref;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkSubpassDependency sd;
    sd.dependencyFlags = 0;
    sd.srcSubpass = VK_SUBPASS_EXTERNAL;
    sd.dstSubpass = 0;
    sd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sd.srcAccessMask = 0;
    sd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    sd.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rp_ci;
    rp_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_ci.pNext = nullptr;
    rp_ci.flags = 0;
    rp_ci.attachmentCount = 2;
    rp_ci.pAttachments = attachments;
    rp_ci.subpassCount = 1;
    rp_ci.pSubpasses = &subpass;
    rp_ci.dependencyCount = 1;
    rp_ci.pDependencies = &sd;

    VkResult rslt = vkCreateRenderPass(device, &rp_ci, nullptr, &m_render_pass);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create render pass. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::TerrainRenderer::cleanupRenderPass() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, m_render_pass, nullptr);
        m_render_pass = VK_NULL_HANDLE;
    }
}

void gfx::TerrainRenderer::initPipelineLayout() {
    if (m_pipeline_layout != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();
    VkDescriptorSetLayout xform_layout = m_system->transformUniforms().descriptorSetLayout();

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

void gfx::TerrainRenderer::cleanupPipelineLayout() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
        m_pipeline_layout = VK_NULL_HANDLE;
    }
}

void gfx::TerrainRenderer::initPipeline() {
    if (m_pipeline != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();
    VkExtent2D extent = m_system->swapchain().extent();

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

    VkVertexInputBindingDescription bind_desc = TerrainVertex::bindingDescription();
    std::array<VkVertexInputAttributeDescription, TerrainVertex::NUM_ATTRIBUTES> attr_desc = TerrainVertex::attributeDescription();

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
    raster_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
    pipeline_ci.renderPass = m_render_pass;
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

void gfx::TerrainRenderer::cleanupPipeline() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
}

void gfx::TerrainRenderer::initFramebuffers(const std::vector<VkImageView> &color_buffers, const DepthBuffer &depth_buffer) {
    if (m_framebuffers.size() > 0) {
        return;
    }

    VkDevice device = m_system->device();
    VkExtent2D extent = m_system->swapchain().extent();

    m_framebuffers.resize(color_buffers.size());
    for (uint32_t i = 0; i < color_buffers.size(); ++i) {
        std::array<VkImageView, 2> attachments{
            color_buffers[i],
            depth_buffer.imageView(),
        };

        VkFramebufferCreateInfo fb_ci;
        fb_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_ci.pNext = nullptr;
        fb_ci.flags = 0;
        fb_ci.renderPass = m_render_pass;
        fb_ci.attachmentCount = attachments.size();
        fb_ci.pAttachments = attachments.data();
        fb_ci.width = extent.width;
        fb_ci.height = extent.height;
        fb_ci.layers = 1;

        VkResult rslt = vkCreateFramebuffer(device, &fb_ci, nullptr, &m_framebuffers[i]);
        if (rslt != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create terrain framebuffer. Error code: " << rslt;
            throw std::runtime_error(msg.str());
        }
    }
}

void gfx::TerrainRenderer::cleanupFramebuffers() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE) {
        for (auto &framebuffer : m_framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    m_framebuffers.clear();
}

template<typename T>
void gfx::TerrainRenderer::initGeometryBuffer(VkBuffer &dst_buffer, VkDeviceMemory &dst_memory, const std::vector<T> &data, VkBufferUsageFlags usage) {
    VkDevice device = m_system->device();
    VkDeviceSize buffer_size = sizeof(T) * data.size();
    VkBuffer staging_buffer{VK_NULL_HANDLE};
    VkDeviceMemory staging_buffer_memory{VK_NULL_HANDLE};
    m_system->createBuffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_buffer_memory);

    void *mapped_data = nullptr;
    VkResult rslt = vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &mapped_data);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Cannot map staging buffer memory. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    std::memcpy(mapped_data, data.data(), buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);

    m_system->createBuffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        dst_buffer, dst_memory);

    m_system->copyBuffer(dst_buffer, staging_buffer, buffer_size);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);
}

void gfx::TerrainRenderer::cleanupGeometryBuffers() {
    VkDevice device = m_system->device();
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
