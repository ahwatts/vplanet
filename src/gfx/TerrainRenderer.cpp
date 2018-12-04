// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <sstream>
#include "../vulkan.h"
#include "System.h"
#include "TerrainRenderer.h"

const Resource TERRAIN_VERT_BYTECODE = LOAD_RESOURCE(terrain_vert_spv);
const Resource TERRAIN_FRAG_BYTECODE = LOAD_RESOURCE(terrain_frag_spv);

gfx::TerrainRenderer::TerrainRenderer(System *system)
    : m_system{system},
      m_vertex_shader{VK_NULL_HANDLE},
      m_fragment_shader{VK_NULL_HANDLE},
      m_render_pass{VK_NULL_HANDLE},
      m_descriptor_set_layout{VK_NULL_HANDLE},
      m_pipeline_layout{VK_NULL_HANDLE}
{}

gfx::TerrainRenderer::~TerrainRenderer() {
    dispose();
}

void gfx::TerrainRenderer::init() {
    if (m_system == nullptr || m_system->device() == VK_NULL_HANDLE) {
        std::stringstream msg;
        msg << "Cannot initialize terrain renderer before system";
        throw std::runtime_error(msg.str());
    }

    initShaderModules();
    initRenderPass();
    initDescriptorSetLayout();
    initPipelineLayout();
}

void gfx::TerrainRenderer::dispose() {
    cleanupShaderModules();
    cleanupRenderPass();
    cleanupDescriptorSetLayout();
    cleanupPipelineLayout();
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
    VkFormat color_format = m_system->swapchainFormat().format;
    VkFormat depth_format = m_system->depthFormat();

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

void gfx::TerrainRenderer::initDescriptorSetLayout() {
    if (m_descriptor_set_layout != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();

    VkDescriptorSetLayoutBinding binding;
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo dsl_ci;
    dsl_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_ci.pNext = nullptr;
    dsl_ci.flags = 0;
    dsl_ci.bindingCount = 1;
    dsl_ci.pBindings = &binding;

    VkResult rslt = vkCreateDescriptorSetLayout(device, &dsl_ci, nullptr, &m_descriptor_set_layout);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create descriptor set layout. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::TerrainRenderer::cleanupDescriptorSetLayout() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptor_set_layout, nullptr);
        m_descriptor_set_layout = VK_NULL_HANDLE;
    }
}

void gfx::TerrainRenderer::initPipelineLayout() {
    if (m_pipeline_layout != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();

    VkPipelineLayoutCreateInfo pl_ci;
    pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_ci.pNext = nullptr;
    pl_ci.flags = 0;
    pl_ci.setLayoutCount = 1;
    pl_ci.pSetLayouts = &m_descriptor_set_layout;
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
