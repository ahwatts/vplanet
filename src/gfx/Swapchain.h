// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_SWAPCHAIN_H_
#define _VPLANET_GFX_SWAPCHAIN_H_

#include <vector>
#include "../vulkan.h"

namespace gfx {
    class System;

    class Swapchain {
    public:
        Swapchain();
        Swapchain(System *system);
        
        const vk::raii::SwapchainKHR& swapchain() const;
        const std::vector<vk::Image>& images() const;
        const std::vector<vk::raii::ImageView>& imageViews() const;
        uint32_t imageCount() const;
        vk::SurfaceFormatKHR format() const;
        vk::Extent2D extent() const;

        void transitionImageToColorAttachment(const vk::raii::CommandBuffer &cmd_buf, uint32_t image_index) const;
        void transitionImageToPresentable(const vk::raii::CommandBuffer &cmd_buf, uint32_t image_index) const;

    private:
        void initSwapchain();
        void initImageViews();

        System *m_system;
        vk::raii::SwapchainKHR m_swapchain;
        std::vector<vk::Image> m_images;
        std::vector<vk::raii::ImageView> m_image_views;
        vk::SurfaceFormatKHR m_format;
        vk::Extent2D m_extent;
    };
}

#endif
