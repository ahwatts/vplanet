// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_SYSTEM_H_
#define _VPLANET_GFX_SYSTEM_H_

#include <vector>
#include "../vulkan.h"

namespace gfx {
    class System {
    public:
        System(GLFWwindow *window);
        ~System();

        void init(bool debug);
        void dispose();

        GLFWwindow* window() const;
        VkInstance instance() const;
        VkDevice device() const;
        VkPhysicalDevice physicalDevice() const;
        VkSurfaceKHR surface() const;
        uint32_t graphicsQueueFamily() const;
        uint32_t presentQueueFamily() const;
        VkSwapchainKHR swapchain() const;
        VkSurfaceFormatKHR swapchainFormat() const;
        VkExtent2D swapchainExtent() const;
        const std::vector<VkImage>& swapchainImages() const;
        const std::vector<VkImageView>& swapchainImageViews() const;
        VkFormat depthFormat() const;

    private:
        void initInstance(bool debug);
        void cleanupInstance();

        void initDebugCallback();
        void cleanupDebugCallback();

        void initSurface();
        void cleanupSurface();

        void initDevice(bool debug);
        void cleanupDevice();

        void initSwapchain();
        void cleanupSwapchain();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT object_type,
            uint64_t object,
            size_t location,
            int32_t code,
            const char *layer_prefix,
            const char *message,
            void *user_data);
        VkBool32 debugCallback(
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT object_type,
            uint64_t object,
            size_t location,
            int32_t code,
            const char *layer_prefix,
            const char *message);

        GLFWwindow *m_window;
        VkInstance m_instance;
        VkDebugReportCallbackEXT m_debug_callback;
        VkSurfaceKHR m_surface;
        VkDevice m_device;
        VkPhysicalDevice m_physical_device;
        uint32_t m_graphics_queue_family, m_present_queue_family;
        VkSwapchainKHR m_swapchain;
        VkSurfaceFormatKHR m_swapchain_format;
        VkExtent2D m_swapchain_extent;
        std::vector<VkImage> m_swapchain_images;
        std::vector<VkImageView> m_swapchain_image_views;
        VkFormat m_depth_format;
    };
};

#endif
