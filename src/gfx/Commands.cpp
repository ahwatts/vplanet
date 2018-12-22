// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <sstream>
#include "../vulkan.h"
#include "Commands.h"
#include "System.h"

gfx::Commands::Commands(System *system)
    : m_system{system},
      m_graphics_queue{VK_NULL_HANDLE},
      m_present_queue{VK_NULL_HANDLE},
      m_pool{VK_NULL_HANDLE},
      m_draw_commands{}
{}

gfx::Commands::~Commands() {
    dispose();
}

void gfx::Commands::init() {
    initQueues();
    initPool();
    initCommandBuffers();
}

void gfx::Commands::dispose() {
    cleanupCommandBuffers();
    cleanupPool();
    cleanupQueues();
}

VkQueue gfx::Commands::graphicsQueue() const {
    return m_graphics_queue;
}

void gfx::Commands::waitGraphicsIdle() const {
    VkResult rslt = vkQueueWaitIdle(m_graphics_queue);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Error waiting for graphics queue to be idle. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

VkQueue gfx::Commands::presentQueue() const {
    return m_present_queue;
}

void gfx::Commands::waitPresentIdle() const {
    VkResult rslt = vkQueueWaitIdle(m_present_queue);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Error waiting for present queue to be idle. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

const std::vector<VkCommandBuffer>& gfx::Commands::drawCommands() const {
    return m_draw_commands;
}

VkCommandBuffer gfx::Commands::beginOneShot() const {
    VkDevice device = m_system->device();

    VkCommandBufferAllocateInfo cb_ai;
    cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_ai.pNext = nullptr;
    cb_ai.commandPool = m_pool;
    cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_ai.commandBufferCount = 1;

    VkCommandBuffer buffer;
    VkResult rslt = vkAllocateCommandBuffers(device, &cb_ai, &buffer);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to allocate one-shot command buffer. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    VkCommandBufferBeginInfo cb_bi;
    cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cb_bi.pNext = nullptr;
    cb_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cb_bi.pInheritanceInfo = nullptr;

    rslt = vkBeginCommandBuffer(buffer, &cb_bi);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to start recording one-shot command buffer. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    return buffer;
}

void gfx::Commands::endOneShot(VkCommandBuffer buffer) const {
    VkDevice device = m_system->device();
    VkResult rslt = vkEndCommandBuffer(buffer);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to finish recording one-shot command buffer. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    VkSubmitInfo si;
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext = nullptr;
    si.waitSemaphoreCount = 0;
    si.pWaitSemaphores = nullptr;
    si.pWaitDstStageMask = nullptr;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &buffer;
    si.signalSemaphoreCount = 0;
    si.pSignalSemaphores = nullptr;

    rslt = vkQueueSubmit(m_graphics_queue, 1, &si, VK_NULL_HANDLE);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to submit one-shot command buffer. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    rslt = vkQueueWaitIdle(m_graphics_queue);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Error waiting for one-shot command buffer to complete. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    vkFreeCommandBuffers(device, m_pool, 1, &buffer);
}

void gfx::Commands::initQueues() {
    VkDevice device = m_system->device();
    uint32_t graphics_queue_family = m_system->graphicsQueueFamily();
    vkGetDeviceQueue(device, graphics_queue_family, 0, &m_graphics_queue);
    uint32_t present_queue_family = m_system->presentQueueFamily();
    vkGetDeviceQueue(device, present_queue_family, 0, &m_present_queue);
}

void gfx::Commands::cleanupQueues() {
    m_graphics_queue = VK_NULL_HANDLE;
    m_present_queue = VK_NULL_HANDLE;
}

void gfx::Commands::initPool() {
    if (m_pool != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();
    uint32_t graphics_queue_family = m_system->graphicsQueueFamily();

    VkCommandPoolCreateInfo cp_ci;
    cp_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cp_ci.pNext = nullptr;
    cp_ci.flags = 0;
    cp_ci.queueFamilyIndex = graphics_queue_family;

    VkResult rslt = vkCreateCommandPool(device, &cp_ci, nullptr, &m_pool);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create command buffer pool. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::Commands::cleanupPool() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, m_pool, nullptr);
        m_pool = VK_NULL_HANDLE;
    }
}

void gfx::Commands::initCommandBuffers() {
    if (m_draw_commands.size() > 0) {
        return;
    }

    VkDevice device = m_system->device();
    uint32_t num_buffers = m_system->swapchain().images().size();
    m_draw_commands.resize(num_buffers, VK_NULL_HANDLE);

    VkCommandBufferAllocateInfo cb_ai;
    cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_ai.pNext = nullptr;
    cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_ai.commandPool = m_pool;
    cb_ai.commandBufferCount = num_buffers;

    VkResult rslt = vkAllocateCommandBuffers(device, &cb_ai, m_draw_commands.data());
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to allocate command buffers. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::Commands::cleanupCommandBuffers() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE && m_pool != VK_NULL_HANDLE && m_draw_commands.size() > 0) {
        vkFreeCommandBuffers(device, m_pool, m_draw_commands.size(), m_draw_commands.data());
    }
}
