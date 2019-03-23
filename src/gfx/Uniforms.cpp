// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <cstring>
#include <sstream>
#include <vector>

#include "../vulkan.h"

#include "../glm_defines.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "System.h"
#include "Uniforms.h"

gfx::Uniforms::Uniforms(System *system)
    : m_system{system},
      m_descriptor_pool{VK_NULL_HANDLE}
{}

gfx::Uniforms::~Uniforms() {
    dispose();
}

void gfx::Uniforms::init() {
    SceneUniformSet::initDescriptorSetLayout(m_system);
    ModelUniformSet::initDescriptorSetLayout(m_system);
    initDescriptorPool();
}

void gfx::Uniforms::dispose() {
    cleanupDescriptorPool();
    ModelUniformSet::cleanupDescriptorSetLayout(m_system);
    SceneUniformSet::cleanupDescriptorSetLayout(m_system);
}

gfx::System* gfx::Uniforms::system() {
    return m_system;
}

VkDescriptorPool gfx::Uniforms::descriptorPool() const {
    return m_descriptor_pool;
}

void gfx::Uniforms::initDescriptorPool() {
    if (m_descriptor_pool != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();
    uint32_t num_images = static_cast<uint32_t>(m_system->swapchain().images().size());
    uint32_t num_descriptors = 4 * num_images;

    std::array<VkDescriptorPoolSize, 1> pool_sizes{};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = num_descriptors;

    VkDescriptorPoolCreateInfo dp_ci;
    dp_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp_ci.pNext = nullptr;
    dp_ci.flags = 0;
    dp_ci.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    dp_ci.pPoolSizes = pool_sizes.data();
    dp_ci.maxSets = num_descriptors;

    VkResult rslt = vkCreateDescriptorPool(device, &dp_ci, nullptr, &m_descriptor_pool);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create descriptor pool. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::Uniforms::cleanupDescriptorPool() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptor_pool, nullptr);
        m_descriptor_pool = VK_NULL_HANDLE;
    }
}

gfx::UniformSet::UniformSet(Uniforms *uniforms)
    : m_uniforms{uniforms},
      m_descriptor_sets{}
{}

gfx::UniformSet::~UniformSet() {
    dispose();
}

void gfx::UniformSet::init() {
    initDescriptorSets();
}

void gfx::UniformSet::dispose() {
    cleanupDescriptorSets();
}

const std::vector<VkDescriptorSet>& gfx::UniformSet::descriptorSets() const {
    return m_descriptor_sets;
}

void gfx::UniformSet::cleanupDescriptorSets() {
    // VkDevice device = m_system->device();
    // if (device != VK_NULL_HANDLE && m_descriptor_pool != VK_NULL_HANDLE && m_descriptor_sets.size() > 0) {
    //     vkFreeDescriptorSets(device, m_descriptor_pool, m_descriptor_sets.size(), m_descriptor_sets.data());
    // }
    m_descriptor_sets.clear();
}

VkDescriptorSetLayout gfx::SceneUniformSet::c_descriptor_set_layout = VK_NULL_HANDLE;

gfx::SceneUniformSet::SceneUniformSet(Uniforms *uniforms)
    : UniformSet(uniforms),
      m_view_projection{},
      m_lights{},
      m_view_projection_buffers{},
      m_view_projection_buffer_memories{},
      m_light_list_buffers{},
      m_light_list_buffer_memories{}
{
    m_view_projection.projection = glm::mat4x4(1.0);
    m_view_projection.view = glm::mat4x4(1.0);

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        m_lights[i].enabled = 0;
        m_lights[i].direction = glm::vec3(0.0, 0.0, 0.0);
    }
}

gfx::SceneUniformSet::~SceneUniformSet() {
    dispose();
}

void gfx::SceneUniformSet::init() {
    initUniformBuffers();
    UniformSet::init();
}

void gfx::SceneUniformSet::dispose() {
    UniformSet::dispose();
    cleanupUniformBuffers();
}

void gfx::SceneUniformSet::initDescriptorSetLayout(System *gfx) {
    if (c_descriptor_set_layout != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = gfx->device();

    std::array<VkDescriptorSetLayoutBinding, 2> bindings;
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_ALL;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_ALL;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo dsl_ci;
    dsl_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_ci.pNext = nullptr;
    dsl_ci.flags = 0;
    dsl_ci.bindingCount = static_cast<uint32_t>(bindings.size());
    dsl_ci.pBindings = bindings.data();

    VkResult rslt = vkCreateDescriptorSetLayout(device, &dsl_ci, nullptr, &c_descriptor_set_layout);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create descriptor set layout. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::SceneUniformSet::cleanupDescriptorSetLayout(System *gfx) {
    VkDevice device = gfx->device();
    if (device != VK_NULL_HANDLE && c_descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, c_descriptor_set_layout, nullptr);
        c_descriptor_set_layout = VK_NULL_HANDLE;
    }
}

VkDescriptorSetLayout gfx::SceneUniformSet::descriptorSetLayout() {
    return c_descriptor_set_layout;
}

void gfx::SceneUniformSet::setTransforms(const ViewProjectionTransform &xform) {
    m_view_projection = xform;
}

void gfx::SceneUniformSet::updateViewProjectionBuffer(uint32_t buffer_index) {
    VkDevice device = m_uniforms->system()->device();
    void *data;

    VkResult rslt = vkMapMemory(
        device,
        m_view_projection_buffer_memories[buffer_index],
        0, sizeof(ViewProjectionTransform),
        0, &data);

    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to map uniform buffer memory. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    std::memcpy(data, &m_view_projection, sizeof(ViewProjectionTransform));
    vkUnmapMemory(device, m_view_projection_buffer_memories[buffer_index]);
}

void gfx::SceneUniformSet::enableLight(uint32_t index, const glm::vec3 &direction) {
    if (index >= 0 && index < MAX_LIGHTS) {
        m_lights[index].enabled = 1;
        m_lights[index].direction = direction;
    }
}

void gfx::SceneUniformSet::disableLight(uint32_t index) {
    if (index >= 0 && index < MAX_LIGHTS) {
        m_lights[index].enabled = 0;
        m_lights[index].direction = glm::vec3(0.0, 0.0, 0.0);
    }
}

void gfx::SceneUniformSet::updateLightListBuffer(uint32_t buffer_index) {
    VkDevice device = m_uniforms->system()->device();
    void *data;

    VkResult rslt = vkMapMemory(
        device,
        m_light_list_buffer_memories[buffer_index],
        0, sizeof(m_lights),
        0, &data);

    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to map light list buffer memory. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    std::memcpy(data, m_lights, sizeof(m_lights));
    vkUnmapMemory(device, m_light_list_buffer_memories[buffer_index]);
}

void gfx::SceneUniformSet::initUniformBuffers() {
    System *gfx = m_uniforms->system();
    VkDevice device = gfx->device();

    if (m_view_projection_buffers.empty()) {
        uint32_t num_buffers = static_cast<uint32_t>(gfx->swapchain().imageViews().size());
        VkDeviceSize buffer_size = sizeof(ViewProjectionTransform);
        m_view_projection_buffers.resize(num_buffers, VK_NULL_HANDLE);
        m_view_projection_buffer_memories.resize(num_buffers, VK_NULL_HANDLE);
        for (uint32_t i = 0; i < num_buffers; ++i) {
            gfx->createBuffer(
                buffer_size,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_view_projection_buffers[i],
                m_view_projection_buffer_memories[i]);
        }
    }

    if (m_light_list_buffers.empty()) {
        uint32_t num_buffers = static_cast<uint32_t>(gfx->swapchain().imageViews().size());
        VkDeviceSize buffer_size = sizeof(m_lights);
        m_light_list_buffers.resize(num_buffers, VK_NULL_HANDLE);
        m_light_list_buffer_memories.resize(num_buffers, VK_NULL_HANDLE);
        for (uint32_t i = 0; i < num_buffers; ++i) {
            gfx->createBuffer(
                buffer_size,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_light_list_buffers[i],
                m_light_list_buffer_memories[i]);
        }
    }
}

void gfx::SceneUniformSet::cleanupUniformBuffers() {
    VkDevice device = m_uniforms->system()->device();

    if (device != VK_NULL_HANDLE) {
        for (auto buffer : m_view_projection_buffers) {
            if (buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, buffer, nullptr);
            }
        }

        for (auto buffer : m_light_list_buffers) {
            if (buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, buffer, nullptr);
            }
        }

        for (auto memory : m_view_projection_buffer_memories) {
            if (memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, memory, nullptr);
            }
        }

        for (auto memory : m_light_list_buffer_memories) {
            if (memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, memory, nullptr);
            }
        }
    }

    m_view_projection_buffers.clear();
    m_view_projection_buffer_memories.clear();
    m_light_list_buffers.clear();
    m_light_list_buffer_memories.clear();
}

void gfx::SceneUniformSet::initDescriptorSets() {
    if (m_descriptor_sets.size() > 0) {
        return;
    }

    System *gfx = m_uniforms->system();
    VkDevice device = gfx->device();
    uint32_t num_images = static_cast<uint32_t>(gfx->swapchain().images().size());

    std::vector<VkDescriptorSetLayout> layouts{num_images, c_descriptor_set_layout};
    m_descriptor_sets.resize(num_images, VK_NULL_HANDLE);

    VkDescriptorSetAllocateInfo ds_ai;
    ds_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ds_ai.pNext = nullptr;
    ds_ai.descriptorPool = m_uniforms->descriptorPool();
    ds_ai.descriptorSetCount = num_images;
    ds_ai.pSetLayouts = layouts.data();

    VkResult rslt = vkAllocateDescriptorSets(device, &ds_ai, m_descriptor_sets.data());
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to allocate descriptor sets. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    for (uint32_t i = 0; i < num_images; ++i) {
        VkDescriptorBufferInfo dbi0;
        dbi0.buffer = m_view_projection_buffers[i];
        dbi0.offset = 0;
        dbi0.range = sizeof(ViewProjectionTransform);

        std::array<VkWriteDescriptorSet, 2> dsc_writes{};

        dsc_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        dsc_writes[0].pNext = nullptr;
        dsc_writes[0].dstSet = m_descriptor_sets[i];
        dsc_writes[0].dstBinding = 0;
        dsc_writes[0].dstArrayElement = 0;
        dsc_writes[0].descriptorCount = 1;
        dsc_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsc_writes[0].pBufferInfo = &dbi0;
        dsc_writes[0].pImageInfo = nullptr;
        dsc_writes[0].pTexelBufferView = nullptr;

        VkDescriptorBufferInfo dbi1;
        dbi1.buffer = m_light_list_buffers[i];
        dbi1.offset = 0;
        dbi1.range = sizeof(m_lights);

        dsc_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        dsc_writes[1].pNext = nullptr;
        dsc_writes[1].dstSet = m_descriptor_sets[i];
        dsc_writes[1].dstBinding = 1;
        dsc_writes[1].dstArrayElement = 0;
        dsc_writes[1].descriptorCount = 1;
        dsc_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsc_writes[1].pBufferInfo = &dbi1;
        dsc_writes[1].pImageInfo = nullptr;
        dsc_writes[1].pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(dsc_writes.size()), dsc_writes.data(), 0, nullptr);
    }
}

VkDescriptorSetLayout gfx::ModelUniformSet::c_descriptor_set_layout = VK_NULL_HANDLE;

gfx::ModelUniformSet::ModelUniformSet(Uniforms *uniforms)
    : UniformSet(uniforms),
      m_model_transform{1.0},
      m_model_buffers{},
      m_model_buffer_memories{}
{}

gfx::ModelUniformSet::~ModelUniformSet() {
    dispose();
}

void gfx::ModelUniformSet::init() {
    initUniformBuffers();
    UniformSet::init();
}

void gfx::ModelUniformSet::dispose() {
    UniformSet::dispose();
    cleanupUniformBuffers();
}

void gfx::ModelUniformSet::initDescriptorSetLayout(System *gfx) {
    if (c_descriptor_set_layout != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = gfx->device();

    std::array<VkDescriptorSetLayoutBinding, 1> bindings;
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_ALL;
    bindings[0].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo dsl_ci;
    dsl_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_ci.pNext = nullptr;
    dsl_ci.flags = 0;
    dsl_ci.bindingCount = static_cast<uint32_t>(bindings.size());
    dsl_ci.pBindings = bindings.data();

    VkResult rslt = vkCreateDescriptorSetLayout(device, &dsl_ci, nullptr, &c_descriptor_set_layout);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create descriptor set layout. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::ModelUniformSet::cleanupDescriptorSetLayout(System *gfx) {
    VkDevice device = gfx->device();
    if (device != VK_NULL_HANDLE && c_descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, c_descriptor_set_layout, nullptr);
        c_descriptor_set_layout = VK_NULL_HANDLE;
    }
}

VkDescriptorSetLayout gfx::ModelUniformSet::descriptorSetLayout() {
    return c_descriptor_set_layout;
}

void gfx::ModelUniformSet::setTransform(const glm::mat4x4 &model) {
    m_model_transform = model;
}

void gfx::ModelUniformSet::updateModelBuffer(uint32_t buffer_index) {
    VkDevice device = m_uniforms->system()->device();
    void *data;

    VkResult rslt = vkMapMemory(
        device,
        m_model_buffer_memories[buffer_index],
        0, sizeof(m_model_transform),
        0, &data);

    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to map light list buffer memory. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    std::memcpy(data, glm::value_ptr(m_model_transform), sizeof(m_model_transform));
    vkUnmapMemory(device, m_model_buffer_memories[buffer_index]);
}

void gfx::ModelUniformSet::initUniformBuffers() {
    System *gfx = m_uniforms->system();
    VkDevice device = gfx->device();

    if (m_model_buffers.empty()) {
        uint32_t num_buffers = static_cast<uint32_t>(gfx->swapchain().imageViews().size());
        VkDeviceSize buffer_size = sizeof(m_model_transform);
        m_model_buffers.resize(num_buffers, VK_NULL_HANDLE);
        m_model_buffer_memories.resize(num_buffers, VK_NULL_HANDLE);
        for (uint32_t i = 0; i < num_buffers; ++i) {
            gfx->createBuffer(
                buffer_size,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_model_buffers[i],
                m_model_buffer_memories[i]);
        }
    }
}

void gfx::ModelUniformSet::cleanupUniformBuffers() {
    VkDevice device = m_uniforms->system()->device();

    if (device != VK_NULL_HANDLE) {
        for (auto buffer : m_model_buffers) {
            if (buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, buffer, nullptr);
            }
        }

        for (auto memory : m_model_buffer_memories) {
            if (memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, memory, nullptr);
            }
        }
    }

    m_model_buffers.clear();
    m_model_buffer_memories.clear();
}

void gfx::ModelUniformSet::initDescriptorSets() {
    if (m_descriptor_sets.size() > 0) {
        return;
    }

    System *gfx = m_uniforms->system();
    VkDevice device = gfx->device();
    uint32_t num_images = static_cast<uint32_t>(gfx->swapchain().images().size());

    std::vector<VkDescriptorSetLayout> layouts{num_images, c_descriptor_set_layout};
    m_descriptor_sets.resize(num_images, VK_NULL_HANDLE);

    VkDescriptorSetAllocateInfo ds_ai;
    ds_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ds_ai.pNext = nullptr;
    ds_ai.descriptorPool = m_uniforms->descriptorPool();
    ds_ai.descriptorSetCount = num_images;
    ds_ai.pSetLayouts = layouts.data();

    VkResult rslt = vkAllocateDescriptorSets(device, &ds_ai, m_descriptor_sets.data());
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to allocate descriptor sets. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    for (uint32_t i = 0; i < num_images; ++i) {
        VkDescriptorBufferInfo dbi0;
        dbi0.buffer = m_model_buffers[i];
        dbi0.offset = 0;
        dbi0.range = sizeof(m_model_transform);

        std::array<VkWriteDescriptorSet, 1> dsc_writes{};
        dsc_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        dsc_writes[0].pNext = nullptr;
        dsc_writes[0].dstSet = m_descriptor_sets[i];
        dsc_writes[0].dstBinding = 0;
        dsc_writes[0].dstArrayElement = 0;
        dsc_writes[0].descriptorCount = 1;
        dsc_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsc_writes[0].pBufferInfo = &dbi0;
        dsc_writes[0].pImageInfo = nullptr;
        dsc_writes[0].pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(dsc_writes.size()), dsc_writes.data(), 0, nullptr);
    }
}
