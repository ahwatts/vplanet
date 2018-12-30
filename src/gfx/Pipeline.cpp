// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "../vulkan.h"

#include "Pipeline.h"
#include "Renderer.h"
#include "System.h"

gfx::Pipeline::Pipeline(Renderer *renderer)
    : m_renderer{renderer},
      m_pipeline_layout{VK_NULL_HANDLE},
      m_pipeline{VK_NULL_HANDLE}
{}

gfx::Pipeline::~Pipeline() {
    dispose();
}

void gfx::Pipeline::init() {
    initPipelineLayout();
    initPipeline();
}

void gfx::Pipeline::dispose() {
    cleanupPipeline();
    cleanupPipelineLayout();
}

void gfx::Pipeline::cleanupPipelineLayout() {
    VkDevice device = m_renderer->system()->device();
    if (device != VK_NULL_HANDLE && m_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipeline_layout, nullptr);
        m_pipeline_layout = VK_NULL_HANDLE;
    }
}

void gfx::Pipeline::cleanupPipeline() {
    VkDevice device = m_renderer->system()->device();
    if (device != VK_NULL_HANDLE && m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
}
