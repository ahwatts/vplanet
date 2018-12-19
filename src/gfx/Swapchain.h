// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_SWAPCHAIN_H_
#define _VPLANET_GFX_SWAPCHAIN_H_

#include <vector>
#include "../vulkan.h"

namespace gfx {
    class System;

    class Swapchain {
    public:
        Swapchain(System *system);
        ~Swapchain();

        void init();
        void dispose();

        VkSwapchainKHR swapchain() const;
        const std::vector<VkImage>& images() const;
        const std::vector<VkImageView>& imageViews() const;
        VkSurfaceFormatKHR format() const;
        VkExtent2D extent() const;

    private:
        void initSwapchain();
        void cleanupSwapchain();

        void initImageViews();
        void cleanupImageViews();

        System *m_system;
        VkSwapchainKHR m_swapchain;
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_image_views;
        VkSurfaceFormatKHR m_format;
        VkExtent2D m_extent;
    };
}

#endif
