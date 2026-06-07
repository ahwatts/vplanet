// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_DEPTH_BUFFER_H_
#define _VPLANET_GFX_DEPTH_BUFFER_H_

#include "../vulkan.h"
#include "../VmaUsage.h"

namespace gfx {
    class System;

    class DepthBuffer {
    public:
        DepthBuffer();
        DepthBuffer(System *system);
        DepthBuffer(const DepthBuffer &other) = delete;
        DepthBuffer(DepthBuffer &&other) = delete;
        
        ~DepthBuffer();

        DepthBuffer &operator=(const DepthBuffer &other) = delete;
        DepthBuffer &operator=(DepthBuffer &&other);

        vk::Format format() const;
        const vk::raii::ImageView &imageView() const;
        bool hasStencilComponent() const;

    private:
        void initDepthResources();
        void transitionImageLayout();

        System *m_system;

        vk::Format m_format;
        vk::raii::Image m_image;
        vk::raii::ImageView m_image_view;
        VmaAllocation m_image_allocation;
    };
}

#endif
