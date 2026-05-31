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
        Commands();
        Commands(const Commands &other) = delete;
        Commands(Commands &&other);

        ~Commands();

        Commands &operator=(const Commands &other) = delete;
        Commands &operator=(Commands &&other);

        const vk::raii::Queue &graphicsQueue() const;
        void waitGraphicsIdle() const;
        const vk::raii::Queue &presentQueue() const;
        void waitPresentIdle() const;
        const std::vector<vk::raii::CommandBuffer> &drawCommands() const;

        vk::raii::CommandBuffer beginOneShot() const;
        void endOneShot(vk::raii::CommandBuffer &&buffer) const;

    private:
        void initQueues();
        void initPool();
        void initCommandBuffers();

        System *m_system;

        vk::raii::Queue m_graphics_queue, m_present_queue;
        vk::raii::CommandPool m_pool;
        std::vector<vk::raii::CommandBuffer> m_draw_commands;
    };
}

#endif
