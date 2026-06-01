// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <array>
#include <sstream>
#include "../vulkan.h"
#include "DepthBuffer.h"
#include "System.h"

vk::Format chooseDepthFormat(const vk::raii::PhysicalDevice &device);

gfx::DepthBuffer::DepthBuffer()
: m_system{nullptr},
  m_format{vk::Format::eUndefined},
  m_image{nullptr},
  m_image_view{nullptr},
  m_image_allocation{VK_NULL_HANDLE}
{}

gfx::DepthBuffer::DepthBuffer(System *system) : DepthBuffer() {
    m_system = system;
    initDepthResources();
    transitionImageLayout();
}

gfx::DepthBuffer::DepthBuffer(DepthBuffer &&other) : DepthBuffer() {
    *this = std::move(other);
}

gfx::DepthBuffer::~DepthBuffer() {}

gfx::DepthBuffer &gfx::DepthBuffer::operator=(DepthBuffer &&other) {
    m_system = other.m_system;
    m_format = other.m_format;
    std::swap(m_image, other.m_image);
    std::swap(m_image_view, other.m_image_view);
    std::swap(m_image_allocation, other.m_image_allocation);
    return *this;
}

vk::Format gfx::DepthBuffer::format() const {
    return m_format;
}

const vk::raii::ImageView &gfx::DepthBuffer::imageView() const {
    return m_image_view;
}

bool gfx::DepthBuffer::hasStencilComponent() const {
    return m_format == vk::Format::eD16UnormS8Uint ||
        m_format == vk::Format::eD24UnormS8Uint ||
        m_format == vk::Format::eD32SfloatS8Uint;
}

void gfx::DepthBuffer::initDepthResources() {
    const vk::raii::Device &device = m_system->device();
    const vk::raii::PhysicalDevice physical_device = m_system->physicalDevice();
    vk::Extent2D extent = m_system->swapchain().extent();
    m_format = chooseDepthFormat(physical_device);

    vk::ImageCreateInfo img_ci{
        .imageType = vk::ImageType::e2D,
        .format = m_format,
        .extent = {extent.width, extent.height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };
    // m_image = device.createImage(img_ci);

    VkImage image;
    VmaAllocationCreateInfo alloc_ci{.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE};
    vmaCreateImage(
        m_system->allocator(),
        static_cast<VkImageCreateInfo *>(img_ci),
        &alloc_ci,
        &image,
        &m_image_allocation,
        nullptr
    );
    m_image = vk::raii::Image(device, image);

    vk::ImageViewCreateInfo iv_ci{
        .image = *m_image,
        .viewType = vk::ImageViewType::e2D,
        .format = m_format,
        .components = {
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eDepth,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    m_image_view = device.createImageView(iv_ci);
}

void gfx::DepthBuffer::transitionImageLayout() {
    const Commands &commands = m_system->commands();
    vk::raii::CommandBuffer cb = commands.beginOneShot();

    vk::ImageMemoryBarrier2 imb{
        .srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        .srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        .dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = *m_image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eDepth,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    vk::DependencyInfo dep = vk::DependencyInfo{}.setImageMemoryBarriers(imb);

    cb.pipelineBarrier2(dep);
    commands.endOneShot(std::move(cb));
}

vk::Format chooseDepthFormat(const vk::raii::PhysicalDevice &device) {
    std::array<vk::Format, 3> candidates{
        vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD24UnormS8Uint,
    };

    auto chosen = std::ranges::find_if(
        candidates,
        [&device](vk::Format format) {
            vk::FormatProperties props = device.getFormatProperties(format);
            return static_cast<bool>(props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment);
        }
    );

    return chosen == candidates.end() ? vk::Format::eUndefined : *chosen;
}
