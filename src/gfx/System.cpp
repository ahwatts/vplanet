// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../vulkan.h"

#include "../Terrain.h"
#include "Commands.h"
#include "DepthBuffer.h"
#include "Renderer.h"
#include "Swapchain.h"
#include "System.h"
#include "Uniforms.h"

struct ChosenDeviceInfo {
    const vk::raii::PhysicalDevice *device;
    uint32_t graphics_queue_family;
    uint32_t present_queue_family;
};

ChosenDeviceInfo choosePhysicalDevice(const std::vector<vk::raii::PhysicalDevice> &devices, const vk::raii::SurfaceKHR &surface, bool debug);
std::vector<const char*> requiredInstanceExtensions(bool debug);
std::vector<const char*> requiredInstanceLayers(bool debug);
std::vector<const char*> requiredDeviceExtensions(bool debug);
std::vector<const char*> requiredDeviceLayers(bool debug);

const char *missingRequiredExtension(const std::vector<const char *> &required, const std::vector<vk::ExtensionProperties> &all);
bool hasExtension(const char *needle, const std::vector<vk::ExtensionProperties> &haystack);
const char *missingRequiredLayer(const std::vector<const char *> &required, const std::vector<vk::LayerProperties> &all);
bool hasLayer(const char *needle, const std::vector<vk::LayerProperties> &haystack);

gfx::System::System(GLFWwindow *window, bool debug)
: m_window{window},
  m_debug{debug},
  m_frame_index{0},
  m_context{},
  m_instance{nullptr},
  m_debug_messenger{nullptr},
  m_surface{nullptr},
  m_physical_device{nullptr},
  m_device{nullptr},
  m_graphics_queue_family{UINT32_MAX},
  m_present_queue_family{UINT32_MAX},
  m_render_finished_semaphores{},
  m_present_complete_semaphores{},
  m_draw_fences{},
  m_allocator{VK_NULL_HANDLE},
  m_commands{},
  m_swapchain{},
  m_depth_buffer{this},
  m_renderer{this},
  m_uniforms{this}
{
    initInstance();

    if (m_debug) {
        initDebugCallback();
    }

    initSurface();
    initDevice();
    initAllocator();
    m_swapchain = Swapchain(this);
    initSynchronizationObjects();
    m_commands = Commands(this);
    m_depth_buffer.init();
    m_uniforms.init();
    m_renderer.init();
}

gfx::System::~System() {
    m_commands.waitGraphicsIdle();
    m_commands.waitPresentIdle();

    m_renderer.dispose();
    m_uniforms.dispose();
    m_depth_buffer.dispose();
    cleanupAllocator();
}

GLFWwindow* gfx::System::window() const {
    return m_window;
}

const vk::raii::Instance &gfx::System::instance() const {
    return m_instance;
}

const vk::raii::Device &gfx::System::device() const {
    return m_device;
}

const vk::raii::PhysicalDevice &gfx::System::physicalDevice() const {
    return m_physical_device;
}

const vk::raii::SurfaceKHR &gfx::System::surface() const {
    return m_surface;
}

uint32_t gfx::System::graphicsQueueFamily() const {
    return m_graphics_queue_family;
}

uint32_t gfx::System::presentQueueFamily() const {
    return m_present_queue_family;
}

VmaAllocator gfx::System::allocator() const {
    return m_allocator;
}

const gfx::Commands& gfx::System::commands() const {
    return m_commands;
}

const gfx::Swapchain& gfx::System::swapchain() const {
    return m_swapchain;
}

const gfx::DepthBuffer& gfx::System::depthBuffer() const {
    return m_depth_buffer;
}

const gfx::Renderer& gfx::System::renderer() const {
    return m_renderer;
}

gfx::Uniforms& gfx::System::uniforms() {
    return m_uniforms;
}

void gfx::System::setTerrainGeometry(const std::vector<TerrainVertex> &verts, const std::vector<uint32_t> &elems) {
    m_renderer.terrainPipeline().setGeometry(verts, elems);
}

void gfx::System::setTerrainTransform(const glm::mat4x4 &xform) {
    m_renderer.terrainPipeline().setTransform(xform);
}

void gfx::System::writeTerrainTransform(uint32_t buffer_index) {
    m_renderer.terrainPipeline().writeTransform(buffer_index);
}

void gfx::System::setOceanGeometry(const std::vector<OceanVertex> &verts, const std::vector<uint32_t> &indices) {
    m_renderer.oceanPipeline().setGeometry(verts, indices);
}

void gfx::System::setOceanTransform(const glm::mat4x4 &xform) {
    m_renderer.oceanPipeline().setTransform(xform);
}

void gfx::System::writeOceanTransform(uint32_t buffer_index) {
    m_renderer.oceanPipeline().writeTransform(buffer_index);
}

void gfx::System::setViewProjectionTransform(const ViewProjectionTransform &xform) {
    m_renderer.setViewProjectionTransform(xform);
}

void gfx::System::writeViewProjectionTransform(uint32_t buffer_index) {
    m_renderer.writeViewProjectionTransform(buffer_index);
}

void gfx::System::enableLight(uint32_t index, const glm::vec3 &direction) {
    m_renderer.enableLight(index, direction);
}

void gfx::System::disableLight(uint32_t index) {
    m_renderer.disableLight(index);
}

void gfx::System::writeLightList(uint32_t buffer_index) {
    m_renderer.writeLightList(buffer_index);
}

void gfx::System::recordCommandBuffers() {
    const std::vector<vk::raii::CommandBuffer> &draw_commands = m_commands.drawCommands();

    for (uint32_t i = 0; i < draw_commands.size(); ++i) {
        const vk::raii::CommandBuffer &commands = draw_commands[i];
        vk::CommandBufferBeginInfo cb_bi{.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse};
        commands.begin(cb_bi);
        m_renderer.recordCommands(*commands, i);
        commands.end();
    }
}

uint32_t gfx::System::startFrame() {
    uint32_t image_index = UINT32_MAX;

    vk::raii::Semaphore &present_complete = m_present_complete_semaphores[m_frame_index];
    vk::raii::Fence &draw_fence = m_draw_fences[m_frame_index];

    vk::Result rslt = m_device.waitForFences(*draw_fence, vk::True, UINT64_MAX);
    if (rslt != vk::Result::eSuccess) {
        std::cerr << "Failed to wait for draw fence to clear: " << vk::to_string(rslt) << "\n";
    }

    rslt = vk::Result(vkAcquireNextImageKHR(
        *m_device,
        *m_swapchain.swapchain(),
        UINT64_MAX,
        *present_complete,
        VK_NULL_HANDLE,
        &image_index
    ));
    if (rslt == vk::Result::eErrorOutOfDateKHR) {
        // re-create swapchain?
    } else if (rslt != vk::Result::eSuccess && rslt != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("Error acquiring next swapchain image");
    }

    m_device.resetFences(*draw_fence);
    return image_index;
}

void gfx::System::drawFrame(uint32_t image_index) {
    vk::raii::Semaphore &present_complete = m_present_complete_semaphores[m_frame_index];    
    vk::raii::Semaphore &render_finished = m_render_finished_semaphores[image_index];
    vk::raii::Fence &draw_fence = m_draw_fences[m_frame_index];
    const std::vector<vk::raii::CommandBuffer> &draw_commands = m_commands.drawCommands();

    vk::PipelineStageFlags wait_dst_stage_mask{vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo si = vk::SubmitInfo{.pWaitDstStageMask = &wait_dst_stage_mask}
        .setWaitSemaphores(*present_complete)
        .setCommandBuffers(*draw_commands[image_index])
        .setSignalSemaphores(*render_finished);
    m_commands.graphicsQueue().submit(si, *draw_fence);
}

void gfx::System::presentFrame(uint32_t image_index) {
    vk::raii::Semaphore &render_finished = m_render_finished_semaphores[image_index];
    const vk::raii::SwapchainKHR &swapchain = m_swapchain.swapchain();

    std::array<VkSemaphore, 1> wait_semaphores{*render_finished};
    vk::PresentInfoKHR pi = vk::PresentInfoKHR{}
        .setWaitSemaphores(*render_finished)
        .setSwapchains(*swapchain)
        .setImageIndices(image_index);
    vk::Result rslt = m_commands.presentQueue().presentKHR(pi);

    if (rslt == vk::Result::eSuboptimalKHR || rslt == vk::Result::eErrorOutOfDateKHR) {
        // re-create swap chain?
    } else if (rslt != vk::Result::eSuccess) {
        throw std::runtime_error("Error presenting new image");
    }
    
    m_frame_index = (m_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

void gfx::System::waitIdle() {
    m_commands.waitGraphicsIdle();
    m_commands.waitPresentIdle();
}

uint32_t gfx::System::chooseMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(*m_physical_device, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if (type_filter & (1 << i) &&
            (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return UINT32_MAX;
}

void gfx::System::createBuffer(
    size_t size,
    VkBufferUsageFlags usage,
    VmaAllocationCreateFlags allocation_flags,
    VkBuffer &buffer,
    VmaAllocation &allocation)
{
    VkBufferCreateInfo buf_ci{};
    buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_ci.size = size;
    buf_ci.usage = usage;
    buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_ci{};
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_ci.flags = allocation_flags;

    VkResult rslt = vmaCreateBuffer(m_allocator, &buf_ci, &alloc_ci, &buffer, &allocation, nullptr);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create buffer. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::System::createBufferWithData(
    const void *data,
    size_t size,
    VkBufferUsageFlags usage,
    VkBuffer &buffer,
    VmaAllocation &allocation)
{
    // Create a staging buffer.
    VkBuffer staging_buffer{VK_NULL_HANDLE};
    VmaAllocation staging_alloc{VK_NULL_HANDLE};
    createBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, 
        staging_buffer, 
        staging_alloc);


    // Create the real buffer.
    createBuffer(
        size,
        usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        0,
        buffer,
        allocation);

    // Copy the data to the staging buffer.
    VkResult rslt = vmaCopyMemoryToAllocation(m_allocator, data, staging_alloc, 0, size);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg{};
        msg << "UNable to copy data to the staging buffer. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    // Copy the staging buffer to the real buffer.
    copyBuffer(buffer, staging_buffer, size);

    // Clean up the staging buffer.
    destroyBuffer(staging_buffer, staging_alloc);
}

void gfx::System::copyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size) {
    vk::BufferCopy region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    vk::raii::CommandBuffer cb = m_commands.beginOneShot();
    cb.copyBuffer(src, dst, region);
    m_commands.endOneShot(std::move(cb));
}

void gfx::System::destroyBuffer(VkBuffer buffer, VmaAllocation allocation) {
    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, buffer, allocation);
    }
}

void gfx::System::createShaderModule(const std::vector<unsigned char> &rsrc, VkShaderModule &shader) {
    VkShaderModuleCreateInfo sm_ci;
    sm_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm_ci.pNext = nullptr;
    sm_ci.flags = 0;
    sm_ci.codeSize = rsrc.size();
    sm_ci.pCode = reinterpret_cast<const uint32_t*>(rsrc.data());

    VkShaderModule module{VK_NULL_HANDLE};
    VkResult rslt = vkCreateShaderModule(*m_device, &sm_ci, nullptr, &shader);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create shader module. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::System::initInstance() {
    std::vector<const char*> required_extensions = requiredInstanceExtensions(m_debug);
    std::vector<vk::ExtensionProperties> extensions = m_context.enumerateInstanceExtensionProperties();
    const char *missing_ext = missingRequiredExtension(required_extensions, extensions);
    if (missing_ext != nullptr) {
        throw std::runtime_error(std::format("Required instance extension {} not supported", missing_ext));
    }

    std::vector<const char *> required_layers = requiredInstanceLayers(m_debug);
    std::vector<vk::LayerProperties> layers = m_context.enumerateInstanceLayerProperties();
    const char *missing_layer = missingRequiredLayer(required_layers, layers);
    if (missing_layer != nullptr) {
        throw std::runtime_error(std::format("Required instance layer {} not supported", missing_layer));
    }

    vk::ApplicationInfo app_info{
        .pApplicationName = "vplanet",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "vplanet-engine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = vk::ApiVersion14,
    };

    vk::InstanceCreateFlags flags;
    // In addition to the extension that was checked for and added above, we
    // also need to pass this flag in.
    #ifdef __APPLE__
    flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    #endif

    vk::InstanceCreateInfo inst_ci = vk::InstanceCreateInfo{
        .flags = flags,
        .pApplicationInfo = &app_info,
    }
        .setPEnabledExtensionNames(required_extensions)
        .setPEnabledLayerNames(required_layers);
    
    m_instance = m_context.createInstance(inst_ci);
}

void gfx::System::initDebugCallback() {
    assert(m_instance != nullptr);

    vk::DebugUtilsMessengerCreateInfoEXT dum_ci{
        .messageSeverity = 
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
        .messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        .pfnUserCallback = gfx::System::debugCallback,
        .pUserData = this,
    };

    m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(dum_ci);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL gfx::System::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT types,
    const vk::DebugUtilsMessengerCallbackDataEXT *data,
    void *user_data
) {
    if (user_data != nullptr) {
        return static_cast<System*>(user_data)->debugCallback(severity, types, data);
    } else {
        return vk::False;
    }
}

vk::Bool32 gfx::System::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT types,
    const vk::DebugUtilsMessengerCallbackDataEXT *data
) {
    std::string msg = std::format(
        "Vulkan {} {}: {}",
        vk::to_string(severity),
        vk::to_string(types), 
        data->pMessage
    );

    std::cerr << msg << "\n";

    // The rest of this is not necessarily all that interesting, but I leave it
    // here in case it is at some point.
    
    // std::cerr << "flags: " << vk::to_string(data->flags) << "\n";
    // std::cerr << "message id name: " << data->pMessageIdName << "\n";
    // std::cerr << "message id number: " << data->messageIdNumber << "\n";

    // std::cerr << "Queue labels: " << data->queueLabelCount << "\n";
    // for (int i = 0; i < data->queueLabelCount; ++i) {
    //     const vk::DebugUtilsLabelEXT &ql = data->pQueueLabels[i];
    //     std::cerr << "color: " << ql.color[0] << " " << ql.color[1] << " " << ql.color[2] << " " << ql.color[3] << " " << ql.pLabelName << "\n";
    // }
    
    // std::cerr << "Command Buffer Labels: " << data->cmdBufLabelCount << "\n";
    // for (int i = 0; i < data->cmdBufLabelCount; ++i) {
    //     const vk::DebugUtilsLabelEXT &bl = data->pCmdBufLabels[i];
    //     std::cerr << "color: " << bl.color[0] << " " << bl.color[1] << " " << bl.color[2] << " " << bl.color[3] << " " << bl.pLabelName << "\n";        
    // }
    
    // std::cerr << "Object Names: " << data->objectCount << "\n";
    // for (int i = 0; i < data->objectCount; ++i) {
    //     const vk::DebugUtilsObjectNameInfoEXT &obj = data->pObjects[i];
    //     std::cerr << vk::to_string(obj.objectType) << " " << obj.objectHandle;
    //     if (obj.pObjectName != nullptr) {
    //         std::cerr << " " << obj.pObjectName;
    //     }
    //     std::cerr << "\n\n";
    // }
    
    return vk::False;
}

VKAPI_ATTR void VKAPI_CALL gfx::System::memoryAllocationCallback(
    VmaAllocator allocator,
    uint32_t memoryType,
    VkDeviceMemory memory,
    VkDeviceSize size,
    void *user_data)
{
    if (user_data != nullptr) {
        static_cast<System*>(user_data)->memoryAllocationCallback(allocator, memoryType, memory, size);
    }
}

void gfx::System::memoryAllocationCallback(
    VmaAllocator allocator,
    uint32_t memoryType,
    VkDeviceMemory memory,
    VkDeviceSize size
) {
    VkMemoryPropertyFlags flags_ = 0;
    vmaGetMemoryTypeProperties(allocator, memoryType, &flags_);
    vk::MemoryPropertyFlags flags{flags_};

    std::cerr << "VMA Allocator: Allocating " << size << " bytes of " << vk::to_string(flags) << " memory\n";
}

VKAPI_ATTR void VKAPI_CALL gfx::System::memoryFreeCallback(
    VmaAllocator allocator,
    uint32_t memoryType,
    VkDeviceMemory memory,
    VkDeviceSize size,
    void *user_data
) {
    if (user_data != nullptr) {
        static_cast<System*>(user_data)->memoryFreeCallback(allocator, memoryType, memory, size);
    }
}

void gfx::System::memoryFreeCallback(
    VmaAllocator allocator,
    uint32_t memoryType,
    VkDeviceMemory memory,
    VkDeviceSize size
) {
    VkMemoryPropertyFlags flags_ = 0;
    vmaGetMemoryTypeProperties(allocator, memoryType, &flags_);
    vk::MemoryPropertyFlags flags{flags_};

    std::cerr << "VMA Allocator: Freeing " << size << " bytes of " << vk::to_string(flags) << " memory\n";
}

void gfx::System::initSurface() {
    assert(m_instance != nullptr);

    VkSurfaceKHR surface{VK_NULL_HANDLE};
    VkResult rslt = glfwCreateWindowSurface(*m_instance, m_window, nullptr, &surface);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create window surface. Error code: " << rslt;
        throw std::runtime_error{msg.str()};
    }

    m_surface = vk::raii::SurfaceKHR(m_instance, surface);
}

void gfx::System::initDevice() {
    assert(m_instance != nullptr);

    std::vector<vk::raii::PhysicalDevice> devices = m_instance.enumeratePhysicalDevices();
    ChosenDeviceInfo chosen_device = choosePhysicalDevice(devices, m_surface, m_debug);
    if (chosen_device.device == nullptr) {
        throw std::runtime_error("Unable to find a suitable physical device");
    }
    m_physical_device = *chosen_device.device;
    m_graphics_queue_family = chosen_device.graphics_queue_family;
    m_present_queue_family = chosen_device.present_queue_family;
    
    float queue_priority = 1.0;
    std::vector<vk::DeviceQueueCreateInfo> queue_cis{
        vk::DeviceQueueCreateInfo{
            .queueFamilyIndex = m_graphics_queue_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        },
    };

    if (m_graphics_queue_family != m_present_queue_family) {
        queue_cis.emplace_back(vk::DeviceQueueCreateInfo{
            .queueFamilyIndex = m_present_queue_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        });
    }

    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > feature_chain = {
        vk::PhysicalDeviceFeatures2{
            .features = vk::PhysicalDeviceFeatures{
                .samplerAnisotropy = true, // Enable ansiotropic filtering in samplers
            },
        },
        vk::PhysicalDeviceVulkan11Features{
            // .shaderDrawParameters = true, // Enable shader draw parameters (we need this for SV_VertexID in the shader)
        },
        vk::PhysicalDeviceVulkan13Features{
            .synchronization2 = true, // Support new synchronization commands
            .dynamicRendering = true, // Enable dynamic rendering from Vulkan 1.3
        },
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{
            .extendedDynamicState = true // Enable extended dynamic state from the extension
        },
    };

    std::vector<const char*> extensions = requiredDeviceExtensions(m_debug);
    std::vector<const char*> layers = requiredDeviceLayers(m_debug);

    vk::DeviceCreateInfo dev_ci = vk::DeviceCreateInfo{.pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>()}
        .setPEnabledExtensionNames(extensions)
        .setQueueCreateInfos(queue_cis);

    m_device = m_physical_device.createDevice(dev_ci);
}

void gfx::System::initSynchronizationObjects() {
    for (int i = 0; i < m_swapchain.imageCount(); ++i) {
        m_render_finished_semaphores.emplace_back(m_device, vk::SemaphoreCreateInfo{});
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_present_complete_semaphores.emplace_back(m_device, vk::SemaphoreCreateInfo{});
        m_draw_fences.emplace_back(m_device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void gfx::System::initAllocator() {
    if (m_allocator == VK_NULL_HANDLE) {
        VmaAllocatorCreateInfo alloc_ci{};
        VmaDeviceMemoryCallbacks callbacks{};

        alloc_ci.physicalDevice = *m_physical_device;
        alloc_ci.device = *m_device;
        alloc_ci.instance = *m_instance;
        alloc_ci.vulkanApiVersion = vk::ApiVersion14;

        if (m_debug) {
            callbacks.pfnAllocate = gfx::System::memoryAllocationCallback;
            callbacks.pfnFree = gfx::System::memoryFreeCallback;
            callbacks.pUserData = this;
            alloc_ci.pDeviceMemoryCallbacks = &callbacks;
        }

        VkResult rslt = vmaCreateAllocator(&alloc_ci, &m_allocator);
        if (rslt != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create memory allocator. Error code: " << rslt;
            throw std::runtime_error{msg.str()};
        }
    }
}

void gfx::System::cleanupAllocator() {
    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }
}

ChosenDeviceInfo choosePhysicalDevice(const std::vector<vk::raii::PhysicalDevice> &devices, const vk::raii::SurfaceKHR &surface, bool debug) {
    for (auto &device : devices) {
        vk::PhysicalDeviceProperties2 props = device.getProperties2();

        const auto features = device.getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        >();
        bool supports_required_features =
            features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
            features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
            features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
        if (!supports_required_features) {
            std::cerr << "Device " << props.properties.deviceName << " doesn't support the required features\n";
            continue;
        }

        uint32_t graphics_family = UINT32_MAX, present_family = UINT32_MAX;
        std::vector<vk::QueueFamilyProperties> queue_families = device.getQueueFamilyProperties();
        for (uint32_t id = 0; id < queue_families.size(); ++id) {
            const vk::QueueFamilyProperties &family = queue_families[id];
            if (graphics_family == UINT32_MAX && (family.queueFlags & vk::QueueFlagBits::eGraphics)) {
                graphics_family = id;
            }

            if (present_family == UINT32_MAX && vk::True == device.getSurfaceSupportKHR(id, *surface)) {
                present_family = id;
            }
        }
        if (graphics_family == UINT32_MAX && present_family == UINT32_MAX) {
            std::cerr << "Device " << props.properties.deviceName << " doesn't have a suitable graphics or present queue\n";
            continue;
        }

        std::vector<const char*> required_extensions = requiredDeviceExtensions(debug);
        std::vector<vk::ExtensionProperties> extensions = device.enumerateDeviceExtensionProperties();
        const char *missing_ext = missingRequiredExtension(required_extensions, extensions);
        if (missing_ext != nullptr) {
            std::cerr << "Device " << props.properties.deviceName << " does not support required extension " << missing_ext << "\n";
            continue;
        }

        std::vector<const char *> required_layers = requiredDeviceLayers(debug);
        std::vector<vk::LayerProperties> layers = device.enumerateDeviceLayerProperties();
        const char *missing_layer = missingRequiredLayer(required_layers, layers);
        if (missing_layer != nullptr) {
            std::cerr << "Device " << props.properties.deviceName << " does not support required layer " << missing_layer << "\n";
            continue;
        }

        std::vector<vk::SurfaceFormatKHR> formats = device.getSurfaceFormatsKHR(*surface);
        std::vector<vk::PresentModeKHR> present_modes = device.getSurfacePresentModesKHR(*surface);
        if (formats.empty() || present_modes.empty()) {
            std::cerr << "Device " << props.properties.deviceName << " has either no surface formats or no surface presentation modes\n";
            continue;
        }

        return {
            &device,
            graphics_family,
            present_family,
        };
    }

    return {
        nullptr,
        UINT32_MAX,
        UINT32_MAX,
    };
}

std::vector<const char*> requiredInstanceExtensions(bool debug) {
    std::vector<const char*> required_extensions;
    
    uint32_t glfw_extension_count;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    for (uint32_t i = 0; i < glfw_extension_count; ++i) {
        required_extensions.push_back(glfw_extensions[i]);
    }

    if (debug) {
        required_extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    #ifdef __APPLE__
    required_extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    #endif

    return required_extensions;
}

std::vector<const char*> requiredInstanceLayers(bool debug) {
    std::vector<const char*> required_layers;

    if (debug) {
        required_layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    return required_layers;
}

std::vector<const char*> requiredDeviceExtensions(bool debug) {
    std::vector<const char*> required_extensions;
    required_extensions.push_back(vk::KHRSwapchainExtensionName);

    #ifdef __APPLE__
    required_extensions.push_back(vk::KHRPortabilitySubsetExtensionName);
    #endif

    return required_extensions;
}

std::vector<const char*> requiredDeviceLayers(bool debug) {
    return std::vector<const char*>{};
}

const char *missingRequiredExtension(const std::vector<const char *> &required, const std::vector<vk::ExtensionProperties> &all) {
    auto rv = std::ranges::find_if(
        required,
        [all](const auto this_ext_name) {
            return !hasExtension(this_ext_name, all);
        }
    );

    if (rv == required.end()) {
        return nullptr;
    } else {
        return *rv;
    }
}

bool hasExtension(const char *needle, const std::vector<vk::ExtensionProperties> &haystack) {
    std::string_view sv_needle{needle};
    return std::ranges::any_of(
        haystack,
        [&sv_needle](const vk::ExtensionProperties &this_ext) {
            std::string_view ext_name{this_ext.extensionName.data()};
            return sv_needle == ext_name;
        }
    );
}

const char *missingRequiredLayer(const std::vector<const char *> &required, const std::vector<vk::LayerProperties> &all) {
    auto rv = std::ranges::find_if(
        required,
        [all](const auto this_layer_name) {
            return !hasLayer(this_layer_name, all);
        }
    );

    if (rv == required.end()) {
        return nullptr;
    } else {
        return *rv;
    }
}

bool hasLayer(const char *needle, const std::vector<vk::LayerProperties> &haystack) {
    std::string_view sv_needle{needle};
    return std::ranges::any_of(
        haystack,
        [&sv_needle](const vk::LayerProperties &this_layer) {
            std::string_view layer_name{this_layer.layerName};
            return sv_needle == layer_name;
        }
    );
}
