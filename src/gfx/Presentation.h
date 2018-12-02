// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_PRESENTATION_H_
#define _VPLANET_GFX_PRESENTATION_H_

#include <vector>
#include "../vulkan.h"

namespace gfx {
    class System;

    class Presentation {
    public:
        Presentation(System *system);
        ~Presentation();

        void init(bool debug);
        void dispose();

    private:
        void initSwapchain();
        void cleanupSwapchain();

        System *m_system;
        VkSwapchainKHR m_swapchain;
        VkSurfaceFormatKHR m_swapchain_format;
        VkExtent2D m_swapchain_extent;
        std::vector<VkImage> m_swapchain_images;
        std::vector<VkImageView> m_swapchain_image_views;
    };
}

#endif
