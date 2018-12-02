// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <vector>
#include "../vulkan.h"
#include "Presentation.h"
#include "System.h"

VkExtent2D chooseSwapchainExtent(GLFWwindow *window, VkSurfaceCapabilitiesKHR &surf_caps);
VkSurfaceFormatKHR chooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR> &formats);
uint32_t chooseImageCount(VkSurfaceCapabilitiesKHR &surf_caps);
VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &modes);

gfx::Presentation::Presentation(System *system)
    : m_system{system},
      m_swapchain{VK_NULL_HANDLE},
      m_swapchain_format{VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
      m_swapchain_extent{0, 0},
      m_swapchain_images{},
      m_swapchain_image_views{}
{}

gfx::Presentation::~Presentation() {
    dispose();
}

void gfx::Presentation::init(bool debug) {
    if (m_system == nullptr ||
        m_system->device() == VK_NULL_HANDLE ||
        m_system->surface() == VK_NULL_HANDLE)
    {
        std::stringstream msg;
        msg << "Cannot initialize presentation before system";
        throw std::runtime_error(msg.str());
    }

    initSwapchain();
}

void gfx::Presentation::dispose() {
    cleanupSwapchain();
}

void gfx::Presentation::initSwapchain() {
    if (m_swapchain != VK_NULL_HANDLE) {
        return;
    }

    GLFWwindow *window = m_system->window();
    VkDevice device = m_system->device();
    VkPhysicalDevice physical_device = m_system->physicalDevice();
    VkSurfaceKHR surface = m_system->surface();
    uint32_t graphics_queue_family = m_system->graphicsQueueFamily();
    uint32_t present_queue_family = m_system->presentQueueFamily();

    VkSurfaceCapabilitiesKHR surf_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surf_caps);

    uint32_t num_surface_formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, nullptr);
    std::vector<VkSurfaceFormatKHR> surface_formats{num_surface_formats};
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, surface_formats.data());

    uint32_t num_present_modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, nullptr);
    std::vector<VkPresentModeKHR> present_modes{num_present_modes};
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, present_modes.data());

    m_swapchain_extent = chooseSwapchainExtent(window, surf_caps);
    m_swapchain_format = chooseSwapchainFormat(surface_formats);
    uint32_t image_count = chooseImageCount(surf_caps);
    VkPresentModeKHR present_mode = choosePresentMode(present_modes);

    std::vector<uint32_t> queue_families{};
    VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    queue_families.push_back(graphics_queue_family);
    if (graphics_queue_family != present_queue_family) {
        queue_families.push_back(present_queue_family);
        sharing_mode = VK_SHARING_MODE_CONCURRENT;
    }

    VkSwapchainCreateInfoKHR swap_ci;
    swap_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_ci.pNext = nullptr;
    swap_ci.flags = 0;
    swap_ci.surface = surface;
    swap_ci.minImageCount = image_count;
    swap_ci.imageFormat = m_swapchain_format.format;
    swap_ci.imageColorSpace = m_swapchain_format.colorSpace;
    swap_ci.imageExtent = m_swapchain_extent;
    swap_ci.imageArrayLayers = 1;
    swap_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_ci.imageSharingMode = sharing_mode;
    swap_ci.queueFamilyIndexCount = static_cast<uint32_t>(queue_families.size());
    swap_ci.pQueueFamilyIndices = queue_families.data();
    swap_ci.preTransform = surf_caps.currentTransform;
    swap_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_ci.presentMode = present_mode;
    swap_ci.clipped = VK_TRUE;
    swap_ci.oldSwapchain = m_swapchain;

    VkResult rslt = vkCreateSwapchainKHR(device, &swap_ci, nullptr, &m_swapchain);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg{};
        msg << "Could not (re-)create swapchain. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    uint32_t num_swapchain_images;
    vkGetSwapchainImagesKHR(device, m_swapchain, &num_swapchain_images, nullptr);
    m_swapchain_images.resize(num_swapchain_images, VK_NULL_HANDLE);
    m_swapchain_image_views.resize(num_swapchain_images, VK_NULL_HANDLE);
    vkGetSwapchainImagesKHR(device, m_swapchain, &num_swapchain_images, m_swapchain_images.data());

    for (int i = 0; i < num_swapchain_images; ++i) {
        VkImageViewCreateInfo iv_ci;
        iv_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv_ci.pNext = nullptr;
        iv_ci.flags = 0;
        iv_ci.image = m_swapchain_images[i];
        iv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv_ci.format = m_swapchain_format.format;
        iv_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv_ci.subresourceRange.baseMipLevel = 0;
        iv_ci.subresourceRange.levelCount = 1;
        iv_ci.subresourceRange.baseArrayLayer = 0;
        iv_ci.subresourceRange.layerCount = 1;

        rslt = vkCreateImageView(device, &iv_ci, nullptr, &m_swapchain_image_views[i]);
        if (rslt != VK_SUCCESS) {
            std::stringstream msg{};
            msg << "Could not create swapchain image view. Error code: " << rslt;
            throw std::runtime_error(msg.str());
        }
    }
}

void gfx::Presentation::cleanupSwapchain() {
    if (m_system != nullptr && m_system->device() != VK_NULL_HANDLE) {
        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_system->device(), m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;
        }

        for (auto &view : m_swapchain_image_views) {
            vkDestroyImageView(m_system->device(), view, nullptr);
        }

        m_swapchain_extent = {0, 0};
        m_swapchain_format = {VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        m_swapchain_images.clear();
        m_swapchain_image_views.clear();
    }
}

VkExtent2D chooseSwapchainExtent(GLFWwindow *window, VkSurfaceCapabilitiesKHR &surf_caps) {
    if (surf_caps.currentExtent.width != UINT32_MAX) {
        return surf_caps.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    extent.width  = std::max(surf_caps.minImageExtent.width,  std::min(surf_caps.maxImageExtent.width,  extent.width));
    extent.height = std::max(surf_caps.minImageExtent.height, std::min(surf_caps.maxImageExtent.height, extent.height));
    return extent;
}

VkSurfaceFormatKHR chooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
    // If the device doesn't care, go with what we want.
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    // If what we want is available, use it.
    for (auto &format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    // Otherwise, go with the first one?
    return formats[0];
}

uint32_t chooseImageCount(VkSurfaceCapabilitiesKHR &surf_caps) {
    uint32_t image_count = surf_caps.minImageCount + 1;
    if (surf_caps.maxImageCount > 0 && image_count > surf_caps.maxImageCount) {
        image_count = surf_caps.maxImageCount;
    }
    return image_count;
}

VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &modes) {
    for (auto &mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
