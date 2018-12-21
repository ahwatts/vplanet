// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <cstring>
#include <sstream>
#include <vector>

#include "../vulkan.h"

#include "../glm_defines.h"
#include <glm/mat4x4.hpp>

#include "System.h"
#include "Uniforms.h"

gfx::XformUniforms::XformUniforms(System *system)
    : m_system{system},
      m_buffers{},
      m_memories{}
{}

gfx::XformUniforms::~XformUniforms() {
    dispose();
}

void gfx::XformUniforms::init() {
    initUniformBuffers();
}

void gfx::XformUniforms::dispose() {
    cleanupUniformBuffers();
}

void gfx::XformUniforms::setTransforms(const Transforms &xforms, uint32_t buffer_index) {
    VkDevice device = m_system->device();
    void *data;

    VkResult rslt = vkMapMemory(device, m_memories[buffer_index], 0, sizeof(Transforms), 0, &data);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to map uniform buffer memory. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    std::memcpy(data, &xforms, sizeof(Transforms));
    vkUnmapMemory(device, m_memories[buffer_index]);
}

void gfx::XformUniforms::initUniformBuffers() {
    if (m_buffers.size() > 0) {
        return;
    }

    VkDevice device = m_system->device();
    uint32_t num_buffers = m_system->swapchain().imageViews().size();
    VkDeviceSize buffer_size = sizeof(Transforms);
    m_buffers.resize(num_buffers, VK_NULL_HANDLE);
    m_memories.resize(num_buffers, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < num_buffers; ++i) {
        m_system->createBuffer(
            buffer_size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_buffers[i], m_memories[i]);
    }
}

void gfx::XformUniforms::cleanupUniformBuffers() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE) {
        for (auto buffer : m_buffers) {
            if (buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, buffer, nullptr);
            }
        }

        for (auto memory : m_memories) {
            if (memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, memory, nullptr);
            }
        }
    }
    m_buffers.clear();
    m_memories.clear();
}
