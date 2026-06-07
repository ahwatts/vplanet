// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_COMMANDS_H_
#define _VPLANET_GFX_COMMANDS_H_

#include <vector>

#include "../vulkan.h"

namespace gfx {
    class System;

    class Commands {
    public:
        Commands();
        Commands(System *system, uint32_t image_count);

        const vk::raii::Queue &graphicsQueue() const;
        void waitGraphicsIdle() const;
        const vk::raii::Queue &presentQueue() const;
        void waitPresentIdle() const;
        const vk::raii::CommandBuffer &commandBuffer(uint32_t image_index) const;
        const std::vector<vk::raii::CommandBuffer> &commandBuffers() const;

        vk::raii::CommandBuffer beginOneShot() const;
        void endOneShot(vk::raii::CommandBuffer &&buffer) const;

    private:
        void initQueues();
        void initPool();
        void initCommandBuffers(uint32_t image_count);

        System *m_system;

        vk::raii::Queue m_graphics_queue, m_present_queue;
        vk::raii::CommandPool m_pool;
        std::vector<vk::raii::CommandBuffer> m_command_buffers;
    };
}

#endif
