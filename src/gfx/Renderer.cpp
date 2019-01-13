// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <sstream>
#include <vector>

#include "../vulkan.h"

#include "Renderer.h"
#include "System.h"

gfx::Renderer::Renderer(System *system)
    : m_system{system},
      m_pipeline_layout{VK_NULL_HANDLE},
      m_render_pass{VK_NULL_HANDLE},
      m_framebuffers{},
      m_uniforms{&system->uniforms()},
      m_ocean_pipeline{this},
      m_terrain_pipeline{this}
{}

gfx::Renderer::~Renderer() {
    dispose();
}

void gfx::Renderer::init() {
    initPipelineLayout();
    initRenderPass();
    initFramebuffers();
    m_uniforms.init();
    m_terrain_pipeline.init();
    m_ocean_pipeline.init();
}

void gfx::Renderer::dispose() {
    m_ocean_pipeline.dispose();
    m_terrain_pipeline.dispose();
    m_uniforms.dispose();
    cleanupFramebuffers();
    cleanupRenderPass();
    cleanupPipelineLayout();
}

gfx::System* gfx::Renderer::system() {
    return m_system;
}


VkPipelineLayout gfx::Renderer::pipelineLayout() const {
    return m_pipeline_layout;
}

VkRenderPass gfx::Renderer::renderPass() const {
    return m_render_pass;
}

gfx::TerrainPipeline& gfx::Renderer::terrainPipeline() {
    return m_terrain_pipeline;
}

gfx::OceanPipeline& gfx::Renderer::oceanPipeline() {
    return m_ocean_pipeline;
}

void gfx::Renderer::setViewProjectionTransform(const ViewProjectionTransform &xform) {
    m_uniforms.setTransforms(xform);
}

void gfx::Renderer::writeViewProjectionTransform(uint32_t buffer_index) {
    m_uniforms.updateViewProjectionBuffer(buffer_index);
}

void gfx::Renderer::enableLight(uint32_t index, const glm::vec3 &direction) {
    m_uniforms.enableLight(index, direction);
}

void gfx::Renderer::disableLight(uint32_t index) {
    m_uniforms.disableLight(index);
}

void gfx::Renderer::writeLightList(uint32_t buffer_index) {
    m_uniforms.updateLightListBuffer(buffer_index);
}

void gfx::Renderer::recordCommands(VkCommandBuffer cmd_buf, uint32_t fb_index) {
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clear_values[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rp_bi;
    rp_bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_bi.pNext = nullptr;
    rp_bi.renderPass = m_render_pass;
    rp_bi.framebuffer = m_framebuffers[fb_index];
    rp_bi.renderArea.offset = { 0, 0 };
    rp_bi.renderArea.extent = m_system->swapchain().extent();
    rp_bi.clearValueCount = clear_values.size();
    rp_bi.pClearValues = clear_values.data();

    const std::vector<VkDescriptorSet> &scene_uniforms = m_uniforms.descriptorSets();

    vkCmdBeginRenderPass(cmd_buf, &rp_bi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &scene_uniforms[fb_index], 0, nullptr);
    m_terrain_pipeline.recordCommands(cmd_buf, fb_index);
    m_ocean_pipeline.recordCommands(cmd_buf, fb_index);
    vkCmdEndRenderPass(cmd_buf);
}

void gfx::Renderer::initPipelineLayout() {
    if (m_pipeline_layout != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();

    std::array<VkDescriptorSetLayout, 2> layouts;
    layouts[0] = SceneUniformSet::descriptorSetLayout();
    layouts[1] = ModelUniformSet::descriptorSetLayout();

    VkPipelineLayoutCreateInfo pl_ci;
    pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_ci.pNext = nullptr;
    pl_ci.flags = 0;
    pl_ci.setLayoutCount = layouts.size();
    pl_ci.pSetLayouts = layouts.data();
    pl_ci.pushConstantRangeCount = 0;
    pl_ci.pPushConstantRanges = nullptr;

    VkResult rslt = vkCreatePipelineLayout(device, &pl_ci, nullptr, &m_pipeline_layout);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create pipeline layout. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::Renderer::cleanupPipelineLayout() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
        m_pipeline_layout = VK_NULL_HANDLE;
    }
}

void gfx::Renderer::initRenderPass() {
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

void gfx::Renderer::cleanupRenderPass() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, m_render_pass, nullptr);
        m_render_pass = VK_NULL_HANDLE;
    }
}

void gfx::Renderer::initFramebuffers() {
    if (m_framebuffers.size() > 0) {
        return;
    }

    VkDevice device = m_system->device();
    VkExtent2D extent = m_system->swapchain().extent();
    const std::vector<VkImageView> &color_buffers = m_system->swapchain().imageViews();
    const DepthBuffer &depth_buffer = m_system->depthBuffer();

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

void gfx::Renderer::cleanupFramebuffers() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE) {
        for (auto &framebuffer : m_framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    m_framebuffers.clear();
}
