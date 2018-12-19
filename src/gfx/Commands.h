// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_COMMANDS_H_
#define _VPLANET_GFX_COMMANDS_H_

#include "../vulkan.h"

namespace gfx {
    class System;

    class Commands {
    public:
        Commands(System *system);
        ~Commands();

        void init();
        void dispose();

        VkCommandBuffer beginOneShot() const;
        void endOneShot(VkCommandBuffer buffer) const;

    private:
        void initQueues();
        void cleanupQueues();

        void initPool();
        void cleanupPool();

        System *m_system;

        VkQueue m_graphics_queue, m_present_queue;
        VkCommandPool m_pool;
    };
}

#endif
