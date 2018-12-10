// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <sstream>
#include "RenderDriver.h"
#include "System.h"

gfx::RenderDriver::RenderDriver(System *system)
    : m_system{system},
      m_depth_image{VK_NULL_HANDLE},
      m_depth_image_view{VK_NULL_HANDLE},
      m_swapchain_images{},
      m_swapchain_image_views{},
      m_framebuffers{},
      m_depth_image_memory{VK_NULL_HANDLE}
{}

gfx::RenderDriver::~RenderDriver() {
    dispose();
}

const std::vector<VkImage>& gfx::RenderDriver::swapchainImages() const {
    return m_swapchain_images;
}

const std::vector<VkImageView>& gfx::RenderDriver::swapchainImageViews() const {
    return m_swapchain_image_views;
}

void gfx::RenderDriver::init() {
    initSwapchainResources();
    initDepthResources();
    initFramebuffers();
}

void gfx::RenderDriver::dispose() {
    cleanupFramebuffers();
    cleanupDepthResources();
    cleanupSwapchainResources();
}

void gfx::RenderDriver::initSwapchainResources() {
    if (m_swapchain_images.size() > 0) {
        return;
    }

    VkDevice device = m_system->device();
    VkSwapchainKHR swapchain = m_system->swapchain();
    VkSurfaceFormatKHR swapchain_format = m_system->swapchainFormat();

    uint32_t num_swapchain_images;
    vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, nullptr);
    m_swapchain_images.resize(num_swapchain_images, VK_NULL_HANDLE);
    m_swapchain_image_views.resize(num_swapchain_images, VK_NULL_HANDLE);
    vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, m_swapchain_images.data());

    for (int i = 0; i < num_swapchain_images; ++i) {
        VkImageViewCreateInfo iv_ci;
        iv_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv_ci.pNext = nullptr;
        iv_ci.flags = 0;
        iv_ci.image = m_swapchain_images[i];
        iv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv_ci.format = swapchain_format.format;
        iv_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv_ci.subresourceRange.baseMipLevel = 0;
        iv_ci.subresourceRange.levelCount = 1;
        iv_ci.subresourceRange.baseArrayLayer = 0;
        iv_ci.subresourceRange.layerCount = 1;

        VkResult rslt = vkCreateImageView(device, &iv_ci, nullptr, &m_swapchain_image_views[i]);
        if (rslt != VK_SUCCESS) {
            std::stringstream msg{};
            msg << "Could not create swapchain image view. Error code: " << rslt;
            throw std::runtime_error(msg.str());
        }
    }
}

void gfx::RenderDriver::cleanupSwapchainResources() {
    VkDevice device = m_system->device();

    if (device != VK_NULL_HANDLE) {
        for (auto &view : m_swapchain_image_views) {
            vkDestroyImageView(device, view, nullptr);
        }

        m_swapchain_images.clear();
        m_swapchain_image_views.clear();
    }
}

void gfx::RenderDriver::initDepthResources() {
    if (m_depth_image_view != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();
    VkFormat depth_format = m_system->depthFormat();
    VkExtent2D extent = m_system->swapchainExtent();

    VkImageCreateInfo img_ci;
    img_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_ci.pNext = nullptr;
    img_ci.flags = 0;
    img_ci.imageType = VK_IMAGE_TYPE_2D;
    img_ci.format = depth_format;
    img_ci.extent = { extent.width, extent.height, 0 };
    img_ci.mipLevels = 1;
    img_ci.arrayLayers = 1;
    img_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    img_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    img_ci.queueFamilyIndexCount = 0;
    img_ci.pQueueFamilyIndices = nullptr;
    img_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult rslt = vkCreateImage(device, &img_ci, nullptr, &m_depth_image);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create depth image. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(device, m_depth_image, &mem_reqs);

    VkMemoryAllocateInfo mem_ai;
    mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.pNext = nullptr;
    mem_ai.allocationSize = mem_reqs.size;
    mem_ai.memoryTypeIndex = m_system->chooseMemoryType(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    rslt = vkAllocateMemory(device, &mem_ai, nullptr, &m_depth_image_memory);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to allocate depth image memory. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    rslt = vkBindImageMemory(device, m_depth_image, m_depth_image_memory, 0);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to bind depth image memory to depth image. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    VkImageViewCreateInfo iv_ci;
    iv_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv_ci.pNext = nullptr;
    iv_ci.flags = 0;
    iv_ci.image = m_depth_image;
    iv_ci.format = depth_format;
    iv_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    iv_ci.subresourceRange.baseMipLevel = 0;
    iv_ci.subresourceRange.levelCount = 1;
    iv_ci.subresourceRange.baseArrayLayer = 0;
    iv_ci.subresourceRange.layerCount = 1;

    rslt = vkCreateImageView(device, &iv_ci, nullptr, &m_depth_image_view);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create depth image view. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    // transition image layout.
}

void gfx::RenderDriver::cleanupDepthResources() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE) {
        if (m_depth_image_view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, m_depth_image_view, nullptr);
            m_depth_image_view = VK_NULL_HANDLE;
        }

        if (m_depth_image != VK_NULL_HANDLE) {
            vkDestroyImage(device, m_depth_image, nullptr);
            m_depth_image = VK_NULL_HANDLE;
        }

        if (m_depth_image_memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_depth_image_memory, nullptr);
            m_depth_image_memory = VK_NULL_HANDLE;
        }
    }
}
