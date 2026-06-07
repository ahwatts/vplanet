// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <algorithm>
#include <iostream>
#include <vector>
#include "../vulkan.h"
#include "Swapchain.h"
#include "System.h"

vk::Extent2D chooseSwapchainExtent(GLFWwindow *window, vk::SurfaceCapabilitiesKHR &surf_caps);
vk::SurfaceFormatKHR chooseSwapchainFormat(const std::vector<vk::SurfaceFormatKHR> &formats);
uint32_t chooseImageCount(vk::SurfaceCapabilitiesKHR &surf_caps);
vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR> &modes);

gfx::Swapchain::Swapchain()
: m_system{nullptr},
  m_swapchain{nullptr},
  m_images{},
  m_image_views{},
  m_format{vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear},
  m_extent{0, 0}
{}

gfx::Swapchain::Swapchain(System *system) : Swapchain() {
    m_system = system;
    initSwapchain();
    initImageViews();
}

// gfx::Swapchain::Swapchain(Swapchain &&other) : Swapchain() {
//     *this = std::move(other);
// }

// gfx::Swapchain::~Swapchain() {}

// gfx::Swapchain &gfx::Swapchain::operator=(Swapchain &&other) {
//     m_system = other.m_system;
//     std::swap(m_swapchain, other.m_swapchain);
//     std::swap(m_images, other.m_images);
//     std::swap(m_image_views, other.m_image_views);
//     m_format = other.m_format;
//     m_extent = other.m_extent;
//     return *this;
// }

const vk::raii::SwapchainKHR &gfx::Swapchain::swapchain() const
{
    return m_swapchain;
}

const std::vector<vk::Image>& gfx::Swapchain::images() const {
    return m_images;
}

const std::vector<vk::raii::ImageView>& gfx::Swapchain::imageViews() const {
    return m_image_views;
}

uint32_t gfx::Swapchain::imageCount() const {
    return static_cast<uint32_t>(m_images.size());
}

vk::SurfaceFormatKHR gfx::Swapchain::format() const {
    return m_format;
}

vk::Extent2D gfx::Swapchain::extent() const {
    return m_extent;
}

void gfx::Swapchain::transitionImageToColorAttachment(const vk::raii::CommandBuffer &cmd_buf, uint32_t image_index) const {
    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .srcAccessMask = {},
        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = m_images[image_index],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vk::DependencyInfo dep = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);

    cmd_buf.pipelineBarrier2(dep);
}

void gfx::Swapchain::transitionImageToPresentable(const vk::raii::CommandBuffer &cmd_buf, uint32_t image_index) const {
    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
        .dstAccessMask = {},
        .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = m_images[image_index],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vk::DependencyInfo dep = vk::DependencyInfo{}.setImageMemoryBarriers(barrier);

    cmd_buf.pipelineBarrier2(dep);
}

void gfx::Swapchain::initSwapchain() {
    GLFWwindow *window = m_system->window();
    const vk::raii::Device &device = m_system->device();
    const vk::raii::PhysicalDevice &physical_device = m_system->physicalDevice();
    const vk::raii::SurfaceKHR &surface = m_system->surface();
    uint32_t graphics_queue_family = m_system->graphicsQueueFamily();
    uint32_t present_queue_family = m_system->presentQueueFamily();

    vk::SurfaceCapabilitiesKHR surf_caps = physical_device.getSurfaceCapabilitiesKHR(*surface);
    std::vector<vk::SurfaceFormatKHR> formats = physical_device.getSurfaceFormatsKHR(*surface);
    std::vector<vk::PresentModeKHR> modes = physical_device.getSurfacePresentModesKHR(*surface);

    m_extent = chooseSwapchainExtent(window, surf_caps);
    m_format = chooseSwapchainFormat(formats);
    uint32_t image_count = chooseImageCount(surf_caps);
    vk::PresentModeKHR present_mode = choosePresentMode(modes);

    std::vector<uint32_t> queue_families{};
    vk::SharingMode sharing_mode = vk::SharingMode::eExclusive;
    queue_families.push_back(graphics_queue_family);
    if (graphics_queue_family != present_queue_family) {
        queue_families.push_back(present_queue_family);
        sharing_mode = vk::SharingMode::eConcurrent;
    }

    vk::SwapchainCreateInfoKHR swap_ci = vk::SwapchainCreateInfoKHR{
        .surface = *surface,
        .minImageCount = image_count,
        .imageFormat = m_format.format,
        .imageColorSpace = m_format.colorSpace,
        .imageExtent = m_extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = sharing_mode,
        .preTransform = surf_caps.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = present_mode,
        .clipped = vk::True,
        .oldSwapchain = *m_swapchain,
    }.setQueueFamilyIndices(queue_families);

    m_swapchain = device.createSwapchainKHR(swap_ci);
    std::cerr << "Created swapchain: " << *m_swapchain << "\n";
}

void gfx::Swapchain::initImageViews() {
    const vk::raii::Device &device = m_system->device();

    vk::ImageViewCreateInfo iv_ci{
        .viewType = vk::ImageViewType::e2D,
        .format = m_format.format,
        .components = {
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    m_images = m_swapchain.getImages();
    for (auto &image : m_images) {
        iv_ci.setImage(image);
        m_image_views.emplace_back(device, iv_ci);
        std::cerr << "Created image view " << *m_image_views.back() << " for swapchain image " << image << "\n";
    }
}

vk::Extent2D chooseSwapchainExtent(GLFWwindow *window, vk::SurfaceCapabilitiesKHR &surf_caps) {
    if (surf_caps.currentExtent.width != UINT32_MAX) {
        return surf_caps.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return {
        std::clamp<uint32_t>(width, surf_caps.minImageExtent.width, surf_caps.maxImageExtent.width),
        std::clamp<uint32_t>(height, surf_caps.minImageExtent.height, surf_caps.maxImageExtent.height),
    };
}

vk::SurfaceFormatKHR chooseSwapchainFormat(const std::vector<vk::SurfaceFormatKHR> &formats) {
    assert(!formats.empty());

    // If the device doesn't care, go with what we want.
    if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) {
        return {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};
    }

    // If what we want is available, use it.
    auto format = std::ranges::find_if(
        formats,
        [](const auto &f) {
            return f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        }
    );

    // Otherwise, go with the first one?
    return format == formats.end() ? formats[0] : *format;
}

uint32_t chooseImageCount(vk::SurfaceCapabilitiesKHR &surf_caps) {
    uint32_t image_count = std::max(3u, surf_caps.minImageCount);
    if (surf_caps.maxImageCount > 0 && image_count > surf_caps.maxImageCount) {
        image_count = surf_caps.maxImageCount;
    }
    return image_count;
}

vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR> &modes) {
    if (std::ranges::contains(modes, vk::PresentModeKHR::eMailbox)) {
        return vk::PresentModeKHR::eMailbox;
    } else {
        return vk::PresentModeKHR::eFifo;
    }
}
