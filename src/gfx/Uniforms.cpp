// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <cstring>
#include <sstream>
#include <vector>

#include "../glm.h"
#include "../vulkan.h"
#include "../VmaUsage.h"

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

// gfx::Uniforms::Uniforms(Uniforms &&other) : Uniforms() {
//     *this = std::move(other);
// }

// gfx::Uniforms::~Uniforms() {}

// gfx::Uniforms &gfx::Uniforms::operator=(Uniforms &&other) {
//     m_system = other.m_system;
//     m_num_frames = other.m_num_frames;
//     std::swap(m_descriptor_pool, other.m_descriptor_pool);
//     std::swap(m_descriptor_set_layouts, other.m_descriptor_set_layouts);
//     return *this;
// }

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
        .descriptorCount = m_num_frames,
    };
    vk::DescriptorPoolCreateInfo dp_ci = vk::DescriptorPoolCreateInfo{
        .maxSets = 4 * m_num_frames,
    }.setPoolSizes(pool_size);

    m_descriptor_pool = device.createDescriptorPool(dp_ci);
}

void gfx::Uniforms::initDescriptorSetLayouts() {
    m_scene_descriptor_set_layout = SceneUniformSet::createDescriptorSetLayout(m_system);
    m_model_descriptor_set_layout = ModelUniformSet::createDescriptorSetLayout(m_system);
}

gfx::UniformSet::UniformSet()
: m_uniforms{nullptr},
  m_descriptor_sets{}
{}

gfx::UniformSet::UniformSet(Uniforms *uniforms) : UniformSet() {
    m_uniforms = uniforms;
}

// gfx::UniformSet::UniformSet(UniformSet &&other) : UniformSet() {
//     *this = std::move(other);
// }

gfx::UniformSet::~UniformSet() {}

// gfx::UniformSet &gfx::UniformSet::operator=(UniformSet &&other) {
//     m_uniforms = other.m_uniforms;
//     std::swap(m_descriptor_sets, other.m_descriptor_sets);
//     return *this;
// }

gfx::Uniforms *gfx::UniformSet::uniforms() {
    return m_uniforms;
}

const std::vector<vk::raii::DescriptorSet> &gfx::UniformSet::descriptorSets() const
{
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
}

// gfx::SceneUniformSet::SceneUniformSet(SceneUniformSet &&other)
// : UniformSet(std::move(other))
// {
//     *this = std::move(other);
// }

gfx::SceneUniformSet::~SceneUniformSet() {
    for (auto &alloc : m_view_projection_buffer_allocations) {
        vmaFreeMemory(m_uniforms->system()->allocator(), alloc);
    }

    for (auto &alloc : m_light_list_buffer_allocations) {
        vmaFreeMemory(m_uniforms->system()->allocator(), alloc);
    }
}

// gfx::SceneUniformSet &gfx::SceneUniformSet::operator=(SceneUniformSet &&other) {
//     m_view_projection = other.m_view_projection;
//     for (int i = 0; i < MAX_LIGHTS; ++i) {
//         m_lights[i].enabled = other.m_lights[i].enabled;
//         m_lights[i].direction = other.m_lights[i].direction;
//     }
//     std::swap(m_view_projection_buffers, other.m_view_projection_buffers);
//     std::swap(m_view_projection_buffer_allocations, other.m_view_projection_buffer_allocations);
//     std::swap(m_light_list_buffers, other.m_light_list_buffers);
//     std::swap(m_light_list_buffer_allocations, other.m_light_list_buffer_allocations);
//     return *this;
// }

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
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
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
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
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

    std::vector<vk::WriteDescriptorSet> writes{};
    for (uint32_t i = 0; i < num_sets; ++i) {
        vk::DescriptorBufferInfo vp_buffer_info{
            .buffer = *m_view_projection_buffers[i],
            .offset = 0,
            .range = sizeof(ViewProjectionTransform),
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
}

// gfx::ModelUniformSet::ModelUniformSet(ModelUniformSet &&other)
// : UniformSet(std::move(other))
// {
//     *this = std::move(other);
// }

gfx::ModelUniformSet::~ModelUniformSet() {
    for (auto &alloc : m_model_buffer_allocations) {
        vmaFreeMemory(m_uniforms->system()->allocator(), alloc);
    }
}

// gfx::ModelUniformSet &gfx::ModelUniformSet::operator=(ModelUniformSet &&other) {
//     m_model_transform = other.m_model_transform;
//     std::swap(m_model_buffers, other.m_model_buffers);
//     std::swap(m_model_buffer_allocations, other.m_model_buffer_allocations);
//     return *this;
// }

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
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
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
