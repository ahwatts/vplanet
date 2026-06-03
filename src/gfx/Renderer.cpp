// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <sstream>
#include <vector>

#include "../vulkan.h"

#include "Renderer.h"
#include "System.h"

gfx::Renderer::Renderer()
: m_system{nullptr},
  m_pipeline_layout{nullptr},
  m_uniforms{},
  m_ocean_pipeline{},
  m_terrain_pipeline{this}
{}

gfx::Renderer::Renderer(System *system) : Renderer() {
    m_system = system;
    initPipelineLayout();
    m_uniforms = SceneUniformSet(&system->uniforms());
    m_ocean_pipeline = OceanPipeline(this);
    // m_terrain_pipeline = TerrainPipeline(this);
}

// gfx::Renderer::~Renderer() {}

// void gfx::Renderer::init() {
//     initPipelineLayout();
//     // initRenderPass();
//     // initFramebuffers();
//     m_uniforms.init();
//     m_terrain_pipeline.init();
//     m_ocean_pipeline.init();
// }

// void gfx::Renderer::dispose() {
//     m_ocean_pipeline.dispose();
//     m_terrain_pipeline.dispose();
//     m_uniforms.dispose();
//     cleanupFramebuffers();
//     cleanupRenderPass();
//     cleanupPipelineLayout();
// }

gfx::System* gfx::Renderer::system() {
    return m_system;
}

const vk::raii::PipelineLayout &gfx::Renderer::pipelineLayout() const {
    return m_pipeline_layout;
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

void gfx::Renderer::recordCommands(const vk::raii::CommandBuffer &cmd_buf, uint32_t image_index, uint32_t frame_index) {
    const DepthBuffer &depth_buffer = m_system->depthBuffer();
    const Swapchain &swapchain = m_system->swapchain();
    vk::Extent2D swapchain_extent = swapchain.extent();

    vk::RenderingAttachmentInfo color_ai{
        .imageView = *swapchain.imageViews()[image_index],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f},
    };
    vk::RenderingAttachmentInfo depth_ai{
        .imageView = *depth_buffer.imageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearDepthStencilValue{1.0f, 0},
    };
    vk::RenderingInfo ri = vk::RenderingInfo{
        .renderArea = {.offset = {0, 0}, .extent = swapchain_extent},
        .layerCount = 1,
        .pDepthAttachment = &depth_ai,
    }.setColorAttachments(color_ai);

    const std::vector<vk::raii::DescriptorSet> &scene_uniforms = m_uniforms.descriptorSets();

    cmd_buf.beginRendering(ri);
    cmd_buf.setViewport(0, vk::Viewport{0.0f, 0.0f, static_cast<float>(swapchain_extent.width), static_cast<float>(swapchain_extent.height)});
    cmd_buf.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, swapchain_extent});
    cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, *scene_uniforms[frame_index], nullptr);
    m_terrain_pipeline.recordCommands(*cmd_buf, frame_index);
    m_ocean_pipeline.recordCommands(cmd_buf, frame_index);
    cmd_buf.endRendering();
}

void gfx::Renderer::initPipelineLayout() {
    const vk::raii::Device &device = m_system->device();

    std::array<vk::DescriptorSetLayout, 2> layouts{
        *SceneUniformSet::descriptorSetLayout(),
        *ModelUniformSet::descriptorSetLayout(),
    };

    vk::PipelineLayoutCreateInfo pl_ci = vk::PipelineLayoutCreateInfo{}
        .setSetLayouts(layouts);
    
    m_pipeline_layout = device.createPipelineLayout(pl_ci);
}
