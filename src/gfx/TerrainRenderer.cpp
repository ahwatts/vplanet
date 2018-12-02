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
      m_fragment_shader{VK_NULL_HANDLE}
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
}

void gfx::TerrainRenderer::dispose() {
    cleanupShaderModules();
}

void gfx::TerrainRenderer::initShaderModules() {
    m_vertex_shader = createShaderModule(TERRAIN_VERT_BYTECODE);
    m_fragment_shader = createShaderModule(TERRAIN_FRAG_BYTECODE);
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
