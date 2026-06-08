// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <cstring>
#include <iostream>
#include <vector>

#include "../glm.h"
#include "../vulkan.h"
#include "../VmaUsage.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/io.hpp>

#include "System.h"
#include "Uniforms.h"

gfx::Uniforms::Uniforms()
: m_system{nullptr},
  m_descriptor_pool{nullptr},
  m_scene_descriptor_set_layout{nullptr},
  m_model_descriptor_set_layout{nullptr},
  m_num_frames{0}
{}

gfx::Uniforms::Uniforms(System *system, uint32_t num_frames) : Uniforms() {
    m_system = system;
    m_num_frames = num_frames;
    initDescriptorSetLayouts();
    initDescriptorPool();
}

gfx::System* gfx::Uniforms::system() {
    return m_system;
}

const vk::raii::DescriptorPool &gfx::Uniforms::descriptorPool() const {
    return m_descriptor_pool;
}

uint32_t gfx::Uniforms::numFrames() const {
    return m_num_frames;
}

const vk::raii::DescriptorSetLayout &gfx::Uniforms::sceneDescriptorSetLayout() const {
    return m_scene_descriptor_set_layout;
}

const vk::raii::DescriptorSetLayout &gfx::Uniforms::modelDescriptorSetLayout() const
{
    return m_model_descriptor_set_layout;
}

void gfx::Uniforms::initDescriptorPool() {
    const vk::raii::Device &device = m_system->device();

    vk::DescriptorPoolSize pool_size{
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 4 * m_num_frames,
    };
    vk::DescriptorPoolCreateInfo dp_ci = vk::DescriptorPoolCreateInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 4 * m_num_frames,
    }.setPoolSizes(pool_size);

    m_descriptor_pool = device.createDescriptorPool(dp_ci);
    std::cerr << "Created descriptor pool: " << *m_descriptor_pool << "\n";
}

void gfx::Uniforms::initDescriptorSetLayouts() {
    m_scene_descriptor_set_layout = SceneUniformSet::createDescriptorSetLayout(m_system);\
    std::cerr << "Created scene uniform descriptor layout: " << *m_scene_descriptor_set_layout << "\n";
    m_model_descriptor_set_layout = ModelUniformSet::createDescriptorSetLayout(m_system);
    std::cerr << "Created model uniform descriptor layout: " << *m_model_descriptor_set_layout << "\n";
}

gfx::UniformSet::UniformSet()
: m_uniforms{nullptr},
  m_descriptor_sets{}
{}

gfx::UniformSet::UniformSet(Uniforms *uniforms) : UniformSet() {
    m_uniforms = uniforms;
}

gfx::UniformSet::~UniformSet() {}

gfx::Uniforms *gfx::UniformSet::uniforms() {
    return m_uniforms;
}

const std::vector<vk::raii::DescriptorSet> &gfx::UniformSet::descriptorSets() const {
    return m_descriptor_sets;
}

gfx::SceneUniformSet::SceneUniformSet()
: UniformSet(),
  m_view_projection{},
  m_lights{},
  m_view_projection_buffers{},
  m_view_projection_buffer_allocations{},
  m_light_list_buffers{},
  m_light_list_buffer_allocations{}
{}

gfx::SceneUniformSet::SceneUniformSet(Uniforms *uniforms)
: UniformSet(uniforms),
  m_view_projection{},
  m_lights{},
  m_view_projection_buffers{},
  m_view_projection_buffer_allocations{},
  m_light_list_buffers{},
  m_light_list_buffer_allocations{}
{
    m_view_projection.projection = glm::mat4x4(1.0);
    m_view_projection.view = glm::mat4x4(1.0);

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        m_lights[i].enabled = 0;
        m_lights[i].direction = glm::vec3(0.0, 0.0, 0.0);
    }

    initUniformBuffers();
    initDescriptorSets();
}

gfx::SceneUniformSet::~SceneUniformSet() {
    if (m_uniforms != nullptr) {
        for (auto &alloc : m_view_projection_buffer_allocations) {
            std::cerr << "Freeing view projection buffer allocation " << alloc << "\n";
            vmaFreeMemory(m_uniforms->system()->allocator(), alloc);
        }

        for (auto &alloc : m_light_list_buffer_allocations) {
            std::cerr << "Freeing light list buffer allocation " << alloc << "\n";
            vmaFreeMemory(m_uniforms->system()->allocator(), alloc);
        }
    }
}

vk::raii::DescriptorSetLayout gfx::SceneUniformSet::createDescriptorSetLayout(System *gfx) {
    const vk::raii::Device &device = gfx->device();

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings{
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eAll,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eAll,
        },
    };

    vk::DescriptorSetLayoutCreateInfo dsl_ci = vk::DescriptorSetLayoutCreateInfo{}
        .setBindings(bindings);

    return device.createDescriptorSetLayout(dsl_ci);
}

const vk::raii::DescriptorSetLayout &gfx::SceneUniformSet::descriptorSetLayout() const {
    return m_uniforms->sceneDescriptorSetLayout();
}

void gfx::SceneUniformSet::setTransforms(const ViewProjectionTransform &xform) {
    m_view_projection = xform;
}

void gfx::SceneUniformSet::updateViewProjectionBuffer(uint32_t buffer_index) {
    VmaAllocator allocator = m_uniforms->system()->allocator();

    VkResult rslt = vmaCopyMemoryToAllocation(
        allocator,
        &m_view_projection,
        m_view_projection_buffer_allocations[buffer_index],
        0, sizeof(ViewProjectionTransform)
    );

    if (rslt != VK_SUCCESS) {
        throw std::runtime_error(
            std::format(
                "Unable to update view / projection buffer. Error code: {}", 
                vk::to_string(vk::Result(rslt))
            )
        );
    }
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
    VmaAllocator allocator = m_uniforms->system()->allocator();

    VkResult rslt = vmaCopyMemoryToAllocation(
        allocator,
        &m_lights,
        m_light_list_buffer_allocations[buffer_index],
        0, sizeof(m_lights)
    );

    if (rslt != VK_SUCCESS) {
        throw std::runtime_error(
            std::format(
                "Unable to update light list buffer. Error code: {}",
                vk::to_string(vk::Result(rslt))
            )
        );
    }
}

void gfx::SceneUniformSet::initUniformBuffers() {
    System *gfx = m_uniforms->system();
    uint32_t num_buffers = m_uniforms->numFrames();

    if (!m_view_projection_buffers.empty()) {
        for (auto &alloc : m_view_projection_buffer_allocations) {
            vmaFreeMemory(gfx->allocator(), alloc);
        }
        m_view_projection_buffers.clear();
        m_view_projection_buffer_allocations.clear();
    }

    vk::DeviceSize vp_buffer_size = sizeof(m_view_projection);
    for (uint32_t i = 0; i < num_buffers; ++i) {
        auto [buffer, allocation] = gfx->createBuffer(
            vp_buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            "view projection uniform"
        );

        m_view_projection_buffers.emplace_back(std::move(buffer));
        m_view_projection_buffer_allocations.push_back(allocation);
    }

    if (!m_light_list_buffers.empty()) {
        for (auto &alloc : m_light_list_buffer_allocations) {
            vmaFreeMemory(gfx->allocator(), alloc);
        }
        m_light_list_buffers.clear();
        m_light_list_buffer_allocations.clear();
    }

    vk::DeviceSize light_list_buffer_size = sizeof(m_lights);
    for (uint32_t i = 0; i < num_buffers; ++i) {
        auto [buffer, allocation] = gfx->createBuffer(
            light_list_buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            "light list uniform"
        );
        m_light_list_buffers.emplace_back(std::move(buffer));
        m_light_list_buffer_allocations.push_back(allocation);
    }
}

void gfx::SceneUniformSet::initDescriptorSets() {
    m_descriptor_sets.clear();

    System *gfx = m_uniforms->system();
    const vk::raii::Device &device = gfx->device();
    uint32_t num_sets = m_uniforms->numFrames();

    std::vector<vk::DescriptorSetLayout> layouts{num_sets, *descriptorSetLayout()};

    vk::DescriptorSetAllocateInfo ds_ai = vk::DescriptorSetAllocateInfo{
        .descriptorPool = *m_uniforms->descriptorPool(),
    }.setSetLayouts(layouts);

    m_descriptor_sets = device.allocateDescriptorSets(ds_ai);
    std::cerr << "Allocated " << num_sets << " descriptor sets for scene uniforms: [ ";
    for (auto &ds : m_descriptor_sets) {
        std::cerr << *ds << " ";
    }
    std::cerr << "]\n";

    std::vector<vk::WriteDescriptorSet> writes{};
    for (uint32_t i = 0; i < num_sets; ++i) {
        vk::DescriptorBufferInfo vp_buffer_info{
            .buffer = *m_view_projection_buffers[i],
            .offset = 0,
            .range = sizeof(m_view_projection),
        };

        vk::WriteDescriptorSet vp_write = vk::WriteDescriptorSet{
            .dstSet = *m_descriptor_sets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
        }.setBufferInfo(vp_buffer_info);

        writes.push_back(vp_write);

        vk::DescriptorBufferInfo light_list_buffer_info{
            .buffer = *m_light_list_buffers[i],
            .offset = 0,
            .range = sizeof(m_lights),
        };

        vk::WriteDescriptorSet light_list_write = vk::WriteDescriptorSet{
            .dstSet = *m_descriptor_sets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
        }.setBufferInfo(light_list_buffer_info);

        writes.push_back(light_list_write);
    }

    device.updateDescriptorSets(writes, {});
}

gfx::ModelUniformSet::ModelUniformSet()
: UniformSet(),
  m_model_transform{},
  m_model_buffers{},
  m_model_buffer_allocations{}
{}

gfx::ModelUniformSet::ModelUniformSet(Uniforms *uniforms)
: UniformSet(uniforms),
    m_model_transform{1.0},
    m_model_buffers{},
    m_model_buffer_allocations{}
{
    initUniformBuffers();
    initDescriptorSets();
}

gfx::ModelUniformSet::~ModelUniformSet() {
    if (m_uniforms != nullptr) {
        for (auto &alloc : m_model_buffer_allocations) {
            vmaFreeMemory(m_uniforms->system()->allocator(), alloc);
        }
    }
}

vk::raii::DescriptorSetLayout gfx::ModelUniformSet::createDescriptorSetLayout(System *gfx) {
    const vk::raii::Device &device = gfx->device();

    vk::DescriptorSetLayoutBinding binding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eAll,
    };

    vk::DescriptorSetLayoutCreateInfo dsl_ci = vk::DescriptorSetLayoutCreateInfo{}
        .setBindings(binding);

    return device.createDescriptorSetLayout(dsl_ci);
}

const vk::raii::DescriptorSetLayout &gfx::ModelUniformSet::descriptorSetLayout() const {
    return m_uniforms->modelDescriptorSetLayout();
}

void gfx::ModelUniformSet::setTransform(const glm::mat4x4 &model) {
    m_model_transform = model;

}

void gfx::ModelUniformSet::updateModelBuffer(uint32_t buffer_index) {
    VmaAllocator allocator = m_uniforms->system()->allocator();

    VkResult rslt = vmaCopyMemoryToAllocation(
        allocator,
        glm::value_ptr(m_model_transform),
        m_model_buffer_allocations[buffer_index],
        0, sizeof(m_model_transform)
    );

    if (rslt != VK_SUCCESS) {
        throw std::runtime_error(
            std::format(
                "Unable to update model transform buffer. Error code: {}",
                vk::to_string(vk::Result(rslt))
            )
        );
    }
}

void gfx::ModelUniformSet::initUniformBuffers() {
    System *gfx = m_uniforms->system();
    uint32_t num_buffers = m_uniforms->numFrames();

    if (!m_model_buffers.empty()) {
        for (auto &alloc : m_model_buffer_allocations) {
            vmaFreeMemory(gfx->allocator(), alloc);
        }
        m_model_buffers.clear();
        m_model_buffer_allocations.clear();
    }

    vk::DeviceSize buffer_size = sizeof(m_model_transform);
    for (uint32_t i = 0; i < num_buffers; ++i) {
        auto [buffer, allocation] = gfx->createBuffer(
            buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            "model uniform"
        );

        m_model_buffers.emplace_back(std::move(buffer));
        m_model_buffer_allocations.push_back(allocation);
    }
}

void gfx::ModelUniformSet::initDescriptorSets() {
    m_descriptor_sets.clear();

    System *gfx = m_uniforms->system();
    const vk::raii::Device &device = gfx->device();
    uint32_t num_sets = m_uniforms->numFrames();

    std::vector<vk::DescriptorSetLayout> layouts{num_sets, *descriptorSetLayout()};

    vk::DescriptorSetAllocateInfo ds_ai = vk::DescriptorSetAllocateInfo{
        .descriptorPool = *m_uniforms->descriptorPool(),
    }.setSetLayouts(layouts);

    m_descriptor_sets = device.allocateDescriptorSets(ds_ai);
    std::cerr << "Allocated " << num_sets << " descriptor sets for model uniforms: [ ";
    for (auto &ds : m_descriptor_sets) {
        std::cerr << *ds << " ";
    }
    std::cerr << "]\n";


    std::vector<vk::WriteDescriptorSet> writes{};
    for (uint32_t i = 0; i < num_sets; ++i) {
        vk::DescriptorBufferInfo vp_buffer_info{
            .buffer = *m_model_buffers[i],
            .offset = 0,
            .range = sizeof(m_model_transform),
        };

        vk::WriteDescriptorSet vp_write = vk::WriteDescriptorSet{
            .dstSet = *m_descriptor_sets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
        }.setBufferInfo(vp_buffer_info);

        writes.push_back(vp_write);
    }

    device.updateDescriptorSets(writes, {});
}
