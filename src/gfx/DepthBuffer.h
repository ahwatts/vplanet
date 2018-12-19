// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_DEPTH_BUFFER_H_
#define _VPLANET_GFX_DEPTH_BUFFER_H_

#include "../vulkan.h"

namespace gfx {
    class System;

    class DepthBuffer {
    public:
        DepthBuffer(System *system);
        ~DepthBuffer();

        VkFormat format() const;
        VkImageView imageView() const;
        bool hasStencilComponent() const;

        void init();
        void dispose();

    private:
        void initDepthResources();
        void cleanupDepthResources();

        void transitionImageLayout();

        System *m_system;

        VkFormat m_format;
        VkImage m_image;
        VkImageView m_image_view;
        VkDeviceMemory m_image_memory;
    };
}

#endif
