// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <algorithm>
#include <cassert>
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
    VkPhysicalDevice device;
    uint32_t graphics_queue_family;
    uint32_t present_queue_family;
};

ChosenDeviceInfo choosePhysicalDevice(const std::vector<VkPhysicalDevice> &devices, VkSurfaceKHR surface, bool debug);
std::vector<const char*> requiredInstanceExtensions(bool debug);
std::vector<const char*> requiredInstanceLayers(bool debug);
std::vector<const char*> requiredDeviceExtensions(bool debug);
std::vector<const char*> requiredDeviceLayers(bool debug);

gfx::System::System(GLFWwindow *window, bool debug)
: m_window{window},
  m_debug{debug},
  m_context{},
  m_instance{nullptr},
  m_debug_messenger{nullptr},
  m_surface{nullptr},
  m_physical_device{VK_NULL_HANDLE},
  m_device{VK_NULL_HANDLE},
  m_graphics_queue_family{UINT32_MAX},
  m_present_queue_family{UINT32_MAX},
  m_image_available_semaphore{VK_NULL_HANDLE},
  m_render_finished_semaphore{VK_NULL_HANDLE},
  m_in_flight_fence{VK_NULL_HANDLE},
  m_allocator{VK_NULL_HANDLE},
  m_commands{this},
  m_swapchain{this},
  m_depth_buffer{this},
  m_renderer{this},
  m_uniforms{this}
{
    initInstance();

    if (m_debug) {
        initDebugCallback();
    }

    initSurface();
    initDevice(debug);
    initAllocator(debug);
    initSemaphores();
    m_swapchain.init();
    m_commands.init();
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
    m_commands.dispose();
    m_swapchain.dispose();
    cleanupSemaphores();
    cleanupAllocator();
    cleanupDevice();
}

GLFWwindow* gfx::System::window() const {
    return m_window;
}

const vk::raii::Instance &gfx::System::instance() const {
    return m_instance;
}

VkDevice gfx::System::device() const {
    return m_device;
}

VkPhysicalDevice gfx::System::physicalDevice() const {
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
    const std::vector<VkCommandBuffer> &draw_commands = m_commands.drawCommands();

    for (uint32_t i = 0; i < draw_commands.size(); ++i) {
        VkCommandBufferBeginInfo cb_bi;
        cb_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cb_bi.pNext = nullptr;
        cb_bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        cb_bi.pInheritanceInfo = nullptr;

        VkResult rslt = vkBeginCommandBuffer(draw_commands[i], &cb_bi);
        if (rslt != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to start recording command buffer. Error code: " << rslt;
            throw std::runtime_error(msg.str());
        }

        m_renderer.recordCommands(draw_commands[i], i);

        rslt = vkEndCommandBuffer(draw_commands[i]);
        if (rslt != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to finish recording command buffer. Error code: " << rslt;
            throw std::runtime_error(msg.str());
        }
    }
}

uint32_t gfx::System::startFrame() {
    uint32_t image_index = UINT32_MAX;

    VkResult rslt = vkWaitForFences(m_device, 1, &m_in_flight_fence, VK_TRUE, UINT64_MAX);
    if (rslt != VK_SUCCESS) {
        std::cerr << "Failed to wait for in-flight fence to clear: " << rslt << std::endl;
    }
    vkResetFences(m_device, 1, &m_in_flight_fence);

    rslt = vkAcquireNextImageKHR(
        m_device,
        m_swapchain.swapchain(),
        UINT64_MAX,
        m_image_available_semaphore,
        VK_NULL_HANDLE,
        &image_index);

    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to get next swapchain image. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    return image_index;
}

void gfx::System::drawFrame(uint32_t image_index) {
    const std::vector<VkCommandBuffer> &draw_commands = m_commands.drawCommands();

    std::array<VkPipelineStageFlags, 1> wait_stages{
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    std::array<VkSemaphore, 1> wait_semaphores{
        m_image_available_semaphore,
    };

    std::array<VkSemaphore, 1> signal_semaphores{
        m_render_finished_semaphore,
    };

    VkSubmitInfo si;
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext = nullptr;
    si.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
    si.pWaitSemaphores = wait_semaphores.data();
    si.pWaitDstStageMask = wait_stages.data();
    si.commandBufferCount = 1;
    si.pCommandBuffers = &draw_commands[image_index];
    si.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
    si.pSignalSemaphores = signal_semaphores.data();

    VkResult rslt = vkQueueSubmit(m_commands.graphicsQueue(), 1, &si, m_in_flight_fence);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to submit command buffer to graphics queue. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::System::presentFrame(uint32_t image_index) {
    VkSwapchainKHR swapchain = m_swapchain.swapchain();

    VkPresentInfoKHR pi;
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.pNext = nullptr;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &m_render_finished_semaphore;
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain;
    pi.pImageIndices = &image_index;
    pi.pResults = nullptr;

    VkResult rslt = vkQueuePresentKHR(m_commands.presentQueue(), &pi);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to submit to present queue. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::System::waitIdle() {
    m_commands.waitGraphicsIdle();
    m_commands.waitPresentIdle();
}

uint32_t gfx::System::chooseMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_props);

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
    VkBufferCopy region{};
    region.srcOffset = 0;
    region.size = size;
    region.dstOffset = 0;

    VkCommandBuffer cb = m_commands.beginOneShot();
    vkCmdCopyBuffer(cb, src, dst, 1, &region);
    m_commands.endOneShot(cb);
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
    VkResult rslt = vkCreateShaderModule(m_device, &sm_ci, nullptr, &shader);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create shader module. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::System::initInstance() {
    std::vector<const char*> required_extensions = requiredInstanceExtensions(m_debug);
    std::vector<vk::ExtensionProperties> extensions = m_context.enumerateInstanceExtensionProperties();

    bool has_required_extensions = std::ranges::all_of(
        required_extensions,
        [&extensions](const char *this_req_ext) {
            std::string_view required_ext_name{this_req_ext};
            bool has_ext = std::ranges::any_of(
                extensions,
                [&required_ext_name](const vk::ExtensionProperties &this_ext) {
                    std::string_view ext_name{this_ext.extensionName.data()};
                    return required_ext_name == ext_name;
                }
            );

            if (!has_ext) {
                std::stringstream msg;
                msg << "Required instance extension " << required_ext_name << " not supported";
                throw std::runtime_error(msg.str());
            } else {
                return true;
            }
        }
    );

    std::vector<const char *> required_layers = requiredInstanceLayers(m_debug);
    std::vector<vk::LayerProperties> layers = m_context.enumerateInstanceLayerProperties();
    bool has_required_layers = std::ranges::all_of(
        required_layers,
        [&layers](const char *this_req_layer) {
            std::string_view required_layer_name{this_req_layer};
            bool has_layer = std::ranges::any_of(
                layers,
                [&required_layer_name](const vk::LayerProperties &this_layer) {
                    std::string_view layer_name{this_layer.layerName};
                    return required_layer_name == layer_name;
                }
            );

            if (!has_layer) {
                std::stringstream msg;
                msg << "Required instance layer " << required_layer_name << " not supported";
                throw std::runtime_error(msg.str());
            } else {
                return true;                
            }
        }
    );

    if (!(has_required_extensions && has_required_layers)) {
        throw std::runtime_error("Not all instance extensions and layers are supported");
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

void printMemoryTypeProperties(VkMemoryPropertyFlags flags) {
    if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        std::cerr << "device local ";
    }
    if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        std::cerr << "host visible ";
    }
    if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        std::cerr << "host coherent ";
    }
    if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
        std::cerr << "host cached ";
    }
    if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
        std::cerr << "lazily allocated ";
    }
    if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT) {
        std::cerr << "protected ";
    }
    if (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) {
        std::cerr << "device coherent (AMD) ";
    }
    if (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) {
        std::cerr << "device uncached (AMD) ";
    }
    if (flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV) {
        std::cerr << "RDMA capable (NV) ";
    }
}

void gfx::System::memoryAllocationCallback(
    VmaAllocator allocator,
    uint32_t memoryType,
    VkDeviceMemory memory,
    VkDeviceSize size)
{
    std::cerr << "VMA Allocator: Allocating " << size << " bytes of ";
    VkMemoryPropertyFlags flags = 0;
    vmaGetMemoryTypeProperties(allocator, memoryType, &flags);
    printMemoryTypeProperties(flags);
    std::cerr << "memory\n";
}

VKAPI_ATTR void VKAPI_CALL gfx::System::memoryFreeCallback(
    VmaAllocator allocator,
    uint32_t memoryType,
    VkDeviceMemory memory,
    VkDeviceSize size,
    void *user_data)
{
    if (user_data != nullptr) {
        static_cast<System*>(user_data)->memoryFreeCallback(allocator, memoryType, memory, size);
    }
}

void gfx::System::memoryFreeCallback(
    VmaAllocator allocator,
    uint32_t memoryType,
    VkDeviceMemory memory,
    VkDeviceSize size)
{
    std::cerr << "VMA Allocator: Freeing " << size << " bytes of ";
    VkMemoryPropertyFlags flags = 0;
    vmaGetMemoryTypeProperties(allocator, memoryType, &flags);
    printMemoryTypeProperties(flags);
    std::cerr << "memory\n";
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

void gfx::System::initDevice(bool debug) {
    if (m_device != VK_NULL_HANDLE) {
        return;
    }

    uint32_t num_devices;
    vkEnumeratePhysicalDevices(*m_instance, &num_devices, nullptr);
    std::vector<VkPhysicalDevice> devices{num_devices};
    vkEnumeratePhysicalDevices(*m_instance, &num_devices, devices.data());
    ChosenDeviceInfo chosen_device = choosePhysicalDevice(devices, *m_surface, debug);
    if (chosen_device.device == VK_NULL_HANDLE) {
        std::stringstream msg;
        msg << "Unable to find suitable physical device";
        throw std::runtime_error{msg.str()};
    }

    float queue_priority = 1.0;
    std::vector<VkDeviceQueueCreateInfo> queue_cis{1};
    queue_cis[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_cis[0].pNext = nullptr;
    queue_cis[0].flags = 0;
    queue_cis[0].queueFamilyIndex = chosen_device.graphics_queue_family;
    queue_cis[0].queueCount = 1;
    queue_cis[0].pQueuePriorities = &queue_priority;

    if (chosen_device.graphics_queue_family != chosen_device.present_queue_family) {
        queue_cis.emplace_back(VkDeviceQueueCreateInfo{});
        queue_cis[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_cis[1].pNext = nullptr;
        queue_cis[1].flags = 0;
        queue_cis[1].queueFamilyIndex = chosen_device.present_queue_family;
        queue_cis[1].queueCount = 1;
        queue_cis[1].pQueuePriorities = &queue_priority;
    }

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;

    std::vector<const char*> extensions = requiredDeviceExtensions(debug);
    std::vector<const char*> layers = requiredDeviceLayers(debug);

    uint32_t num_available_extensions;
    vkEnumerateDeviceExtensionProperties(chosen_device.device, nullptr, &num_available_extensions, nullptr);
    std::vector<VkExtensionProperties> available_extensions{num_available_extensions};
    vkEnumerateDeviceExtensionProperties(chosen_device.device, nullptr, &num_available_extensions, available_extensions.data());

    for (auto extension : available_extensions) {
        if (std::strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0) {
            extensions.push_back("VK_KHR_portability_subset");
        }
    }

    VkDeviceCreateInfo dev_ci;
    dev_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_ci.pNext = nullptr;
    dev_ci.flags = 0;
    dev_ci.pEnabledFeatures = &features;
    dev_ci.queueCreateInfoCount = static_cast<uint32_t>(queue_cis.size());
    dev_ci.pQueueCreateInfos = queue_cis.data();
    dev_ci.enabledLayerCount = static_cast<uint32_t>(layers.size());
    dev_ci.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();
    dev_ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    dev_ci.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

    VkResult rslt = vkCreateDevice(chosen_device.device, &dev_ci, nullptr, &m_device);
    if (rslt == VK_SUCCESS) {
        m_physical_device = chosen_device.device;
        m_graphics_queue_family = chosen_device.graphics_queue_family;
        m_present_queue_family = chosen_device.present_queue_family;
    } else {
        std::stringstream msg;
        msg << "Error creating Vulkan device. Error code: " << rslt;
        throw std::runtime_error{msg.str()};
    }
}

void gfx::System::cleanupDevice() {
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
        m_physical_device = VK_NULL_HANDLE;
        m_graphics_queue_family = UINT32_MAX;
        m_present_queue_family = UINT32_MAX;
    }
}

void gfx::System::initAllocator(bool debug) {
    if (m_allocator == VK_NULL_HANDLE) {
        VmaAllocatorCreateInfo alloc_ci{};
        VmaDeviceMemoryCallbacks callbacks{};

        alloc_ci.physicalDevice = m_physical_device;
        alloc_ci.device = m_device;
        alloc_ci.instance = *m_instance;
        alloc_ci.vulkanApiVersion = VK_API_VERSION_1_0;

        if (debug) {
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

void gfx::System::initSemaphores() {
    if (m_image_available_semaphore == VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo sem_ci{};
        sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        sem_ci.pNext = nullptr;
        sem_ci.flags = 0;
        vkCreateSemaphore(m_device, &sem_ci, nullptr, &m_image_available_semaphore);
    }

    if (m_render_finished_semaphore == VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo sem_ci{};
        sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        sem_ci.pNext = nullptr;
        sem_ci.flags = 0;
        vkCreateSemaphore(m_device, &sem_ci, nullptr, &m_render_finished_semaphore);
    }

    if (m_in_flight_fence == VK_NULL_HANDLE) {
        VkFenceCreateInfo fence_ci{};
        fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_ci.pNext = nullptr;
        fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_device, &fence_ci, nullptr, &m_in_flight_fence);
    }
}

void gfx::System::cleanupSemaphores() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_image_available_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_image_available_semaphore, nullptr);
            m_image_available_semaphore = VK_NULL_HANDLE;
        }

        if (m_render_finished_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_render_finished_semaphore, nullptr);
            m_render_finished_semaphore = VK_NULL_HANDLE;
        }

        if (m_in_flight_fence != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_in_flight_fence, nullptr);
            m_in_flight_fence = VK_NULL_HANDLE;
        }
    }
}

ChosenDeviceInfo choosePhysicalDevice(const std::vector<VkPhysicalDevice> &devices, VkSurfaceKHR surface, bool debug) {
    for (auto &device : devices) {
        // Does it have the properties we want?
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        // Does it support the features we want?
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);
        if (features.samplerAnisotropy != VK_TRUE) {
            std::cerr << "Device " << properties.deviceName << " doesn't support the required features\n";
            continue;
        }

        // Do we have appropriate queue families for graphics / presentation?
        uint32_t num_queue_families, graphics_queue = UINT32_MAX, present_queue = UINT32_MAX;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families{num_queue_families};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, queue_families.data());
        for (uint32_t id = 0; id < queue_families.size(); ++id) {
            if (graphics_queue == UINT32_MAX && queue_families[id].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_queue = id;
            }

            if (present_queue == UINT32_MAX) {
                VkBool32 supports_present;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, id, surface, &supports_present);
                if (supports_present == VK_TRUE) {
                    present_queue = id;
                }
            }

            if (graphics_queue < UINT32_MAX && present_queue < UINT32_MAX) {
                break;
            }
        }
        if (graphics_queue == UINT32_MAX || present_queue == UINT32_MAX) {
            std::cerr << "Device " << properties.deviceName << " doesn't have suitable graphics or present queues\n";
            continue;
        }

        // Are the extensions / layers we want supported?
        std::vector<const char*> wanted_extensions = requiredDeviceExtensions(debug);
        uint32_t num_extensions;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);
        std::vector<VkExtensionProperties> extensions{num_extensions};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, extensions.data());
        bool all_found = true;
        for (auto wanted_extension_name : wanted_extensions) {
            bool found = false;
            std::string wanted_extension_name_str{wanted_extension_name};
            for (auto &extension : extensions) {
                std::string extension_name_str{extension.extensionName};
                if (wanted_extension_name_str == extension_name_str) {
                    found = true;
                    break;
                }
            }

            all_found = found && all_found;
        }
        if (!all_found) {
            std::cerr << "Device " << properties.deviceName << " doesn't support all the required device extensions\n";
            continue;
        }

        std::vector<const char*> wanted_layers = requiredDeviceLayers(debug);
        uint32_t num_layers;
        vkEnumerateDeviceLayerProperties(device, &num_layers, nullptr);
        std::vector<VkLayerProperties> layers{num_layers};
        vkEnumerateDeviceLayerProperties(device, &num_layers, layers.data());
        all_found = true;
        for (auto wanted_layer_name : wanted_layers) {
            bool found = false;
            std::string wanted_layer_name_str{wanted_layer_name};
            for (auto &layer : layers) {
                std::string layer_name_str{layer.layerName};
                if (wanted_layer_name_str == layer_name_str) {
                    found = true;
                    break;
                }
            }

            all_found = found && all_found;
        }
        if (!all_found) {
            std::cerr << "Device " << properties.deviceName << " doesn't support all the required device layers\n";
            continue;
        }

        // Are swapchains supported, and are there surface formats and present modes we can use?
        uint32_t num_formats;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, nullptr);
        uint32_t num_present_modes;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_present_modes, nullptr);
        if (num_formats == 0 || num_present_modes == 0) {
            std::cerr << "Device " << properties.deviceName << " has either no surface formats or no surface presentation modes\n";
            continue;
        }

        return {
            device,
            graphics_queue,
            present_queue,
        };
    }

    return {
        VK_NULL_HANDLE,
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
    required_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
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
    return required_extensions;
}

std::vector<const char*> requiredDeviceLayers(bool debug) {
    return std::vector<const char*>{};
}
