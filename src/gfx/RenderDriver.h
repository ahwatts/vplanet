// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_RENDER_DRIVER_H_
#define _VPLANET_GFX_RENDER_DRIVER_H_

#include <vector>
#include "../vulkan.h"

namespace gfx {
    class System;

    class RenderDriver {
    public:
        RenderDriver(System *system);
        ~RenderDriver();

        void init();
        void dispose();

        const std::vector<VkImage>& swapchainImages() const;
        const std::vector<VkImageView>& swapchainImageViews() const;

    private:
        void initSwapchainResources();
        void cleanupSwapchainResources();

        void initDepthResources();
        void cleanupDepthResources();

        void initFramebuffers();
        void cleanupFramebuffers();

        System *m_system;
        VkImage m_depth_image;
        VkImageView m_depth_image_view;
        std::vector<VkImage> m_swapchain_images;
        std::vector<VkImageView> m_swapchain_image_views;
        std::vector<VkFramebuffer> m_framebuffers;
        VkDeviceMemory m_depth_image_memory;
    };
}

#endif
