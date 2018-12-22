// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_COMMANDS_H_
#define _VPLANET_GFX_COMMANDS_H_

#include <vector>

#include "../vulkan.h"

namespace gfx {
    class System;

    class Commands {
    public:
        Commands(System *system);
        ~Commands();

        void init();
        void dispose();

        VkQueue graphicsQueue() const;
        VkQueue presentQueue() const;
        const std::vector<VkCommandBuffer>& drawCommands() const;

        VkCommandBuffer beginOneShot() const;
        void endOneShot(VkCommandBuffer buffer) const;

    private:
        void initQueues();
        void cleanupQueues();

        void initPool();
        void cleanupPool();

        void initCommandBuffers();
        void cleanupCommandBuffers();

        System *m_system;

        VkQueue m_graphics_queue, m_present_queue;
        VkCommandPool m_pool;
        std::vector<VkCommandBuffer> m_draw_commands;
    };
}

#endif
