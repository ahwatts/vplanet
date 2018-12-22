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
      m_descriptor_set_layout{VK_NULL_HANDLE},
      m_descriptor_pool{VK_NULL_HANDLE},
      m_descriptor_sets{},
      m_buffers{},
      m_memories{}
{}

gfx::XformUniforms::~XformUniforms() {
    dispose();
}

void gfx::XformUniforms::init() {
    initDescriptorSetLayout();
    initDescriptorPool();
    initUniformBuffers();
    initDescriptorSets();
}

void gfx::XformUniforms::dispose() {
    cleanupDescriptorSets();
    cleanupUniformBuffers();
    cleanupDescriptorPool();
    cleanupDescriptorSetLayout();
}

VkDescriptorSetLayout gfx::XformUniforms::descriptorSetLayout() const {
    return m_descriptor_set_layout;
}

const std::vector<VkDescriptorSet>& gfx::XformUniforms::descriptorSets() const {
    return m_descriptor_sets;
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

void gfx::XformUniforms::initDescriptorSetLayout() {
    VkDevice device = m_system->device();

    VkDescriptorSetLayoutBinding binding;
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_ALL;
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

void gfx::XformUniforms::cleanupDescriptorSetLayout() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptor_set_layout, nullptr);
        m_descriptor_set_layout = VK_NULL_HANDLE;
    }
}

void gfx::XformUniforms::initDescriptorPool() {
    if (m_descriptor_pool != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();
    uint32_t num_images = m_system->swapchain().images().size();

    std::array<VkDescriptorPoolSize, 1> pool_sizes{};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = num_images;

    VkDescriptorPoolCreateInfo dp_ci;
    dp_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp_ci.pNext = nullptr;
    dp_ci.flags = 0;
    dp_ci.poolSizeCount = pool_sizes.size();
    dp_ci.pPoolSizes = pool_sizes.data();
    dp_ci.maxSets = num_images;

    VkResult rslt = vkCreateDescriptorPool(device, &dp_ci, nullptr, &m_descriptor_pool);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create descriptor pool. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::XformUniforms::cleanupDescriptorPool() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptor_pool, nullptr);
        m_descriptor_pool = VK_NULL_HANDLE;
    }
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

void gfx::XformUniforms::initDescriptorSets() {
    if (m_descriptor_sets.size() > 0) {
        return;
    }

    VkDevice device = m_system->device();
    uint32_t num_images = m_system->swapchain().images().size();

    std::vector<VkDescriptorSetLayout> layouts{num_images, m_descriptor_set_layout};
    m_descriptor_sets.resize(num_images, VK_NULL_HANDLE);

    VkDescriptorSetAllocateInfo ds_ai;
    ds_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ds_ai.pNext = nullptr;
    ds_ai.descriptorPool = m_descriptor_pool;
    ds_ai.descriptorSetCount = num_images;
    ds_ai.pSetLayouts = layouts.data();

    VkResult rslt = vkAllocateDescriptorSets(device, &ds_ai, m_descriptor_sets.data());
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to allocate descriptor sets. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    for (uint32_t i = 0; i < num_images; ++i) {
        VkDescriptorBufferInfo dbi;
        dbi.buffer = m_buffers[i];
        dbi.offset = 0;
        dbi.range = sizeof(Transforms);

        std::array<VkWriteDescriptorSet, 1> dsc_writes{};

        dsc_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        dsc_writes[0].pNext = nullptr;
        dsc_writes[0].dstSet = m_descriptor_sets[i];
        dsc_writes[0].dstBinding = 0;
        dsc_writes[0].dstArrayElement = 0;
        dsc_writes[0].descriptorCount = 1;
        dsc_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsc_writes[0].pBufferInfo = &dbi;
        dsc_writes[0].pImageInfo = nullptr;
        dsc_writes[0].pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(device, dsc_writes.size(), dsc_writes.data(), 0, nullptr);
    }
}

void gfx::XformUniforms::cleanupDescriptorSets() {
    // VkDevice device = m_system->device();
    // if (device != VK_NULL_HANDLE && m_descriptor_pool != VK_NULL_HANDLE && m_descriptor_sets.size() > 0) {
    //     vkFreeDescriptorSets(device, m_descriptor_pool, m_descriptor_sets.size(), m_descriptor_sets.data());
    // }
    m_descriptor_sets.clear();
}
