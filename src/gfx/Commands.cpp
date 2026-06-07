// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include "../vulkan.h"
#include "Commands.h"
#include "System.h"

gfx::Commands::Commands()
: m_system{nullptr},
  m_graphics_queue{nullptr},
  m_present_queue{nullptr},
  m_pool{nullptr},
  m_command_buffers{}
{}

gfx::Commands::Commands(System *system, uint32_t image_count) : Commands() {
    m_system = system;
    initQueues();
    initPool();
    initCommandBuffers(image_count);
}

const vk::raii::Queue &gfx::Commands::graphicsQueue() const {
    return m_graphics_queue;
}

void gfx::Commands::waitGraphicsIdle() const {
    if (m_graphics_queue != nullptr) {
        m_graphics_queue.waitIdle();
    }
}

const vk::raii::Queue &gfx::Commands::presentQueue() const {
    return m_present_queue;
}

void gfx::Commands::waitPresentIdle() const {
    if (m_present_queue != nullptr) {
        m_present_queue.waitIdle();
    }
}

const vk::raii::CommandBuffer &gfx::Commands::commandBuffer(uint32_t image_index) const {
    return m_command_buffers[image_index];
}

const std::vector<vk::raii::CommandBuffer>& gfx::Commands::commandBuffers() const {
    return m_command_buffers;
}

vk::raii::CommandBuffer gfx::Commands::beginOneShot() const {
    const vk::raii::Device &device = m_system->device();

    vk::CommandBufferAllocateInfo cb_ai{
        .commandPool = *m_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };
    vk::raii::CommandBuffer commands = std::move(device.allocateCommandBuffers(cb_ai).front());

    vk::CommandBufferBeginInfo cb_bi{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commands.begin(cb_bi);
    return std::move(commands);
}

void gfx::Commands::endOneShot(vk::raii::CommandBuffer &&commands) const {
    const vk::raii::Device &device = m_system->device();
    commands.end();
    vk::SubmitInfo si = vk::SubmitInfo{}.setCommandBuffers(*commands);
    m_graphics_queue.submit(si);
    m_graphics_queue.waitIdle();
}

void gfx::Commands::initQueues() {
    const vk::raii::Device &device = m_system->device();
    uint32_t graphics_queue_family = m_system->graphicsQueueFamily();
    m_graphics_queue = device.getQueue(graphics_queue_family, 0);
    std::cerr << "Got graphics queue: " << *m_graphics_queue << "\n";
    uint32_t present_queue_family = m_system->presentQueueFamily();
    m_present_queue = device.getQueue(present_queue_family, 0);
    std::cerr << "Got present queue: " << *m_present_queue << "\n";
}

void gfx::Commands::initPool() {
    const vk::raii::Device &device = m_system->device();
    uint32_t graphics_queue_family = m_system->graphicsQueueFamily();

    vk::CommandPoolCreateInfo cp_ci{.queueFamilyIndex = graphics_queue_family};
    m_pool = device.createCommandPool(vk::CommandPoolCreateInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,       
        .queueFamilyIndex = graphics_queue_family,
    });
    std::cerr << "Created command pool: " << *m_pool << "\n";
}

void gfx::Commands::initCommandBuffers(uint32_t image_count) {
    const vk::raii::Device &device = m_system->device();

    vk::CommandBufferAllocateInfo cb_ai{
        .commandPool = *m_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = image_count,
    };
    m_command_buffers = device.allocateCommandBuffers(cb_ai);
    std::cerr << "Allocated " << m_command_buffers.size() << " command buffers:\n";
    for (auto &cb : m_command_buffers) {
        std::cerr << "  " << *cb << "\n";
    }
}
