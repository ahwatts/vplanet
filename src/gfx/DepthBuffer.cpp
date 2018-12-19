// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <array>
#include <sstream>
#include "../vulkan.h"
#include "DepthBuffer.h"
#include "System.h"

VkFormat chooseDepthFormat(VkPhysicalDevice device);

gfx::DepthBuffer::DepthBuffer(System *system)
    : m_system{system},
      m_format{VK_FORMAT_UNDEFINED},
      m_image{VK_NULL_HANDLE},
      m_image_view{VK_NULL_HANDLE},
      m_image_memory{VK_NULL_HANDLE}
{}

gfx::DepthBuffer::~DepthBuffer() {
    dispose();
}

void gfx::DepthBuffer::init() {
    initDepthResources();
    transitionImageLayout();
}

void gfx::DepthBuffer::dispose() {
    cleanupDepthResources();
}

VkFormat gfx::DepthBuffer::format() const {
    return m_format;
}

VkImageView gfx::DepthBuffer::imageView() const {
    return m_image_view;
}

bool gfx::DepthBuffer::hasStencilComponent() const {
    return m_format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void gfx::DepthBuffer::initDepthResources() {
    if (m_image_view != VK_NULL_HANDLE) {
        return;
    }

    VkDevice device = m_system->device();
    VkPhysicalDevice physical_device = m_system->physicalDevice();
    VkExtent2D extent = m_system->swapchain().extent();
    m_format = chooseDepthFormat(physical_device);

    VkImageCreateInfo img_ci;
    img_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_ci.pNext = nullptr;
    img_ci.flags = 0;
    img_ci.imageType = VK_IMAGE_TYPE_2D;
    img_ci.format = m_format;
    img_ci.extent = { extent.width, extent.height, 1 };
    img_ci.mipLevels = 1;
    img_ci.arrayLayers = 1;
    img_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    img_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    img_ci.queueFamilyIndexCount = 0;
    img_ci.pQueueFamilyIndices = nullptr;
    img_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult rslt = vkCreateImage(device, &img_ci, nullptr, &m_image);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create depth image. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(device, m_image, &mem_reqs);

    VkMemoryAllocateInfo mem_ai;
    mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.pNext = nullptr;
    mem_ai.allocationSize = mem_reqs.size;
    mem_ai.memoryTypeIndex = m_system->chooseMemoryType(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    rslt = vkAllocateMemory(device, &mem_ai, nullptr, &m_image_memory);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to allocate depth image memory. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    rslt = vkBindImageMemory(device, m_image, m_image_memory, 0);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to bind depth image memory to depth image. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    VkImageViewCreateInfo iv_ci;
    iv_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv_ci.pNext = nullptr;
    iv_ci.flags = 0;
    iv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv_ci.image = m_image;
    iv_ci.format = m_format;
    iv_ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    iv_ci.subresourceRange.baseMipLevel = 0;
    iv_ci.subresourceRange.levelCount = 1;
    iv_ci.subresourceRange.baseArrayLayer = 0;
    iv_ci.subresourceRange.layerCount = 1;

    rslt = vkCreateImageView(device, &iv_ci, nullptr, &m_image_view);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create depth image view. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::DepthBuffer::cleanupDepthResources() {
    VkDevice device = m_system->device();
    if (device != VK_NULL_HANDLE) {
        if (m_image_view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, m_image_view, nullptr);
            m_image_view = VK_NULL_HANDLE;
        }

        if (m_image != VK_NULL_HANDLE) {
            vkDestroyImage(device, m_image, nullptr);
            m_image = VK_NULL_HANDLE;
        }

        if (m_image_memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_image_memory, nullptr);
            m_image_memory = VK_NULL_HANDLE;
        }
    }
}

void gfx::DepthBuffer::transitionImageLayout() {
    const Commands &commands = m_system->commands();
    VkCommandBuffer cb = commands.beginOneShot();

    VkImageMemoryBarrier barrier;
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    if (hasStencilComponent()) {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    vkCmdPipelineBarrier(cb, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    commands.endOneShot(cb);
}

VkFormat chooseDepthFormat(VkPhysicalDevice device) {
    std::array<VkFormat, 3> candidates{
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    for (auto format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}
