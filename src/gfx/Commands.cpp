// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <sstream>
#include "../vulkan.h"
#include "Commands.h"
#include "System.h"

gfx::Commands::Commands()
: m_system{nullptr},
  m_graphics_queue{nullptr},
  m_present_queue{nullptr},
  m_pool{nullptr},
  m_draw_commands{}
{}

gfx::Commands::Commands(System *system) : Commands() {
    m_system = system;
    initQueues();
    initPool();
    initCommandBuffers();
}

// gfx::Commands::Commands(Commands &&other) : Commands() {
//     *this = std::move(other);
// }

// gfx::Commands::~Commands() {}

// gfx::Commands &gfx::Commands::operator=(Commands && other) {
//     m_system = other.m_system;
//     std::swap(m_graphics_queue, other.m_graphics_queue);
//     std::swap(m_present_queue, other.m_present_queue);
//     std::swap(m_pool, other.m_pool);
//     std::swap(m_draw_commands, other.m_draw_commands);
//     return *this;
// }

const vk::raii::Queue &gfx::Commands::graphicsQueue() const {
    return m_graphics_queue;
}

void gfx::Commands::waitGraphicsIdle() const {
    m_graphics_queue.waitIdle();
}

const vk::raii::Queue &gfx::Commands::presentQueue() const {
    return m_present_queue;
}

void gfx::Commands::waitPresentIdle() const {
    m_present_queue.waitIdle();
}

const std::vector<vk::raii::CommandBuffer>& gfx::Commands::drawCommands() const {
    return m_draw_commands;
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
    uint32_t present_queue_family = m_system->presentQueueFamily();
    m_present_queue = device.getQueue(present_queue_family, 0);
}

void gfx::Commands::initPool() {
    const vk::raii::Device &device = m_system->device();
    uint32_t graphics_queue_family = m_system->graphicsQueueFamily();

    vk::CommandPoolCreateInfo cp_ci{.queueFamilyIndex = graphics_queue_family};
    m_pool = device.createCommandPool(cp_ci);
}

void gfx::Commands::initCommandBuffers() {
    const vk::raii::Device &device = m_system->device();
    uint32_t num_buffers = m_system->swapchain().imageCount();

    vk::CommandBufferAllocateInfo cb_ai{
        .commandPool = *m_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = num_buffers,
    };
    m_draw_commands = device.allocateCommandBuffers(cb_ai);
}
