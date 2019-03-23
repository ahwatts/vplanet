// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <cstring>
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

gfx::System::System(GLFWwindow *window)
    : m_window{window},
      m_instance{VK_NULL_HANDLE},
      m_debug_callback{VK_NULL_HANDLE},
      m_surface{VK_NULL_HANDLE},
      m_physical_device{VK_NULL_HANDLE},
      m_device{VK_NULL_HANDLE},
      m_graphics_queue_family{UINT32_MAX},
      m_present_queue_family{UINT32_MAX},
      m_image_available_semaphore{VK_NULL_HANDLE},
      m_render_finished_semaphore{VK_NULL_HANDLE},
      m_commands{this},
      m_swapchain{this},
      m_depth_buffer{this},
      m_renderer{this},
      m_uniforms{this}
{}

gfx::System::~System() {
    dispose();
}

void gfx::System::init(bool debug) {
    initInstance(debug);

    if (debug) {
        initDebugCallback();
    }

    initSurface();
    initDevice(debug);
    initSemaphores();
    m_swapchain.init();
    m_commands.init();
    m_depth_buffer.init();
    m_uniforms.init();
    m_renderer.init();
}

void gfx::System::dispose() {
    m_renderer.dispose();
    m_uniforms.dispose();
    m_depth_buffer.dispose();
    m_commands.dispose();
    m_swapchain.dispose();
    cleanupSemaphores();
    cleanupDevice();
    cleanupSurface();
    cleanupDebugCallback();
    cleanupInstance();
}

GLFWwindow* gfx::System::window() const {
    return m_window;
}

VkInstance gfx::System::instance() const {
    return m_instance;
}

VkDevice gfx::System::device() const {
    return m_device;
}

VkPhysicalDevice gfx::System::physicalDevice() const {
    return m_physical_device;
}

VkSurfaceKHR gfx::System::surface() const {
    return m_surface;
}

uint32_t gfx::System::graphicsQueueFamily() const {
    return m_graphics_queue_family;
}

uint32_t gfx::System::presentQueueFamily() const {
    return m_present_queue_family;
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
    VkResult rslt = vkAcquireNextImageKHR(
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

    VkResult rslt = vkQueueSubmit(m_commands.graphicsQueue(), 1, &si, VK_NULL_HANDLE);
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
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags mem_props,
    VkBuffer &buffer,
    VkDeviceMemory &memory)
{
    VkBufferCreateInfo buf_ci;
    buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_ci.pNext = nullptr;
    buf_ci.flags = 0;
    buf_ci.size = size;
    buf_ci.usage = usage;
    buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_ci.queueFamilyIndexCount = 0;
    buf_ci.pQueueFamilyIndices = nullptr;

    VkResult rslt = vkCreateBuffer(m_device, &buf_ci, nullptr, &buffer);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create buffer. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(m_device, buffer, &mem_reqs);

    uint32_t mem_type = chooseMemoryType(mem_reqs.memoryTypeBits, mem_props);
    if (mem_type == UINT32_MAX) {
        std::stringstream msg;
        msg << "No memory type appropriate for buffer";
        throw std::runtime_error(msg.str());
    }

    VkMemoryAllocateInfo mem_ai;
    mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.pNext = nullptr;
    mem_ai.allocationSize = mem_reqs.size;
    mem_ai.memoryTypeIndex = mem_type;

    rslt = vkAllocateMemory(m_device, &mem_ai, nullptr, &memory);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to allocate buffer memory. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    rslt = vkBindBufferMemory(m_device, buffer, memory, 0);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to bind buffer memory to buffer. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }
}

void gfx::System::createBufferWithData(
    const void *data,
    size_t data_size,
    VkBufferUsageFlags usage,
    VkBuffer &dst_buffer,
    VkDeviceMemory &dst_memory)
{
    VkDeviceSize buffer_size = data_size;
    VkBuffer staging_buffer{VK_NULL_HANDLE};
    VkDeviceMemory staging_buffer_memory{VK_NULL_HANDLE};
    createBuffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_buffer_memory);

    void *mapped_data = nullptr;
    VkResult rslt = vkMapMemory(m_device, staging_buffer_memory, 0, buffer_size, 0, &mapped_data);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Cannot map staging buffer memory. Error code: " << rslt;
        throw std::runtime_error(msg.str());
    }

    std::memcpy(mapped_data, data, data_size);
    vkUnmapMemory(m_device, staging_buffer_memory);

    createBuffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        dst_buffer, dst_memory);

    copyBuffer(dst_buffer, staging_buffer, buffer_size);

    vkDestroyBuffer(m_device, staging_buffer, nullptr);
    vkFreeMemory(m_device, staging_buffer_memory, nullptr);
}

void gfx::System::copyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size) {
    VkBufferCopy region;
    region.srcOffset = 0;
    region.size = size;
    region.dstOffset = 0;

    VkCommandBuffer cb = m_commands.beginOneShot();
    vkCmdCopyBuffer(cb, src, dst, 1, &region);
    m_commands.endOneShot(cb);
}

void gfx::System::createShaderModule(const Resource &rsrc, VkShaderModule &shader) {
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

void gfx::System::initInstance(bool debug) {
    if (m_instance != VK_NULL_HANDLE) {
        return;
    }

    std::vector<const char*> wanted_extensions = requiredInstanceExtensions(debug);
    uint32_t num_extensions;
    vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, nullptr);
    std::vector<VkExtensionProperties> extensions{num_extensions};
    vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, extensions.data());
    for (auto &wanted_extension_name : wanted_extensions) {
        bool found = false;
        std::string wanted_extension_name_str{wanted_extension_name};
        for (auto &extension : extensions) {
            std::string extension_name_str{extension.extensionName};
            if (wanted_extension_name_str == extension_name_str) {
                found = true;
                break;
            }
        }

        if (!found) {
            std::stringstream msg;
            msg << "Unable to find instance extension " << wanted_extension_name_str << ". Cannot continue.";
            throw std::runtime_error(msg.str());
        }
    }

    std::vector<const char *> wanted_layers = requiredInstanceLayers(debug);
    uint32_t num_layers;
    vkEnumerateInstanceLayerProperties(&num_layers, nullptr);
    std::vector<VkLayerProperties> layers{num_layers};
    vkEnumerateInstanceLayerProperties(&num_layers, layers.data());
    for (auto &wanted_layer_name : wanted_layers) {
        bool found = false;
        std::string wanted_layer_name_str{wanted_layer_name};
        for (auto &layer : layers) {
            std::string layer_name_str{layer.layerName};
            if (wanted_layer_name_str == layer_name_str) {
                found = true;
                break;
            }
        }

        if (!found) {
            std::stringstream msg;
            msg << "Unable to find layer " << wanted_layer_name_str << ". Cannot continue.";
            throw std::runtime_error{msg.str()};
        }
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = "vplanet";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "vplanet";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_ci{};
    inst_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_ci.pNext = nullptr;
    inst_ci.flags = 0;
    inst_ci.pApplicationInfo = &app_info;
    inst_ci.enabledLayerCount = static_cast<uint32_t>(wanted_layers.size());
    inst_ci.ppEnabledLayerNames = wanted_layers.empty() ? nullptr : wanted_layers.data();
    inst_ci.enabledExtensionCount = static_cast<uint32_t>(wanted_extensions.size());
    inst_ci.ppEnabledExtensionNames = wanted_extensions.empty() ? nullptr : wanted_extensions.data();

    VkResult rslt = vkCreateInstance(&inst_ci, nullptr, &m_instance);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create Vulkan instance. Error code: " << rslt;
        throw std::runtime_error{msg.str()};
    }
}

void gfx::System::cleanupInstance() {
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

void gfx::System::initDebugCallback() {
    if (m_debug_callback != VK_NULL_HANDLE) {
        return;
    }

    VkDebugReportCallbackCreateInfoEXT drc_ci{};
    drc_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    drc_ci.pNext = nullptr;
    drc_ci.flags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
        VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    drc_ci.pUserData = this;
    drc_ci.pfnCallback = gfx::System::debugCallback;

    VkResult rslt = vkCreateDebugReportCallbackEXT(m_instance, &drc_ci, nullptr, &m_debug_callback);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create debug report callback. Error code: " << rslt;
        throw std::runtime_error{msg.str()};
    }
}

void gfx::System::cleanupDebugCallback() {
    if (m_instance != VK_NULL_HANDLE && m_debug_callback != VK_NULL_HANDLE) {
        vkDestroyDebugReportCallbackEXT(m_instance, m_debug_callback, nullptr);
        m_debug_callback = VK_NULL_HANDLE;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL gfx::System::debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT object_type,
    uint64_t object,
    size_t location,
    int32_t code,
    const char *layer_prefix,
    const char *message,
    void *user_data)
{
    if (user_data != nullptr) {
        return static_cast<System*>(user_data)->debugCallback(flags, object_type, object, location, code, layer_prefix, message);
    } else {
        return VK_FALSE;
    }
}

VkBool32 gfx::System::debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT object_type,
    uint64_t object,
    size_t location,
    int32_t code,
    const char *layer_prefix,
    const char *message)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        std::cerr << "Vulkan error: " << layer_prefix << ": " << message << "\n";
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        std::cerr << "Vulkan warning: " << layer_prefix << ": " << message << "\n";
    } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        std::cerr << "Vulkan performance warning: " << layer_prefix << ": " << message << "\n";
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        std::cerr << "Vulkan info: " << layer_prefix << ": " << message << "\n";
    } else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        std::cerr << "Vulkan debug: " << layer_prefix << ": " << message << "\n";
    } else {
        std::cerr << "Vulkan unknown level: " << layer_prefix << ": " << message << "\n";
    }

    return VK_FALSE;
}

void gfx::System::initSurface() {
    if (m_surface != VK_NULL_HANDLE) {
        return;
    }

    VkResult rslt = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
    if (rslt != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create window surface. Error code: " << rslt;
        throw std::runtime_error{msg.str()};
    }
}

void gfx::System::cleanupSurface() {
    if (m_instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
}

void gfx::System::initDevice(bool debug) {
    if (m_device != VK_NULL_HANDLE) {
        return;
    }

    uint32_t num_devices;
    vkEnumeratePhysicalDevices(m_instance, &num_devices, nullptr);
    std::vector<VkPhysicalDevice> devices{num_devices};
    vkEnumeratePhysicalDevices(m_instance, &num_devices, devices.data());
    ChosenDeviceInfo chosen_device = choosePhysicalDevice(devices, m_surface, debug);
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
        queue_cis[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_cis[0].pNext = nullptr;
        queue_cis[0].flags = 0;
        queue_cis[0].queueFamilyIndex = chosen_device.present_queue_family;
        queue_cis[0].queueCount = 1;
        queue_cis[0].pQueuePriorities = &queue_priority;
    }

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;

    std::vector<const char*> extensions = requiredDeviceExtensions(debug);
    std::vector<const char*> layers = requiredDeviceLayers(debug);

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

void gfx::System::initSemaphores() {
    if (m_image_available_semaphore == VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo sem_ci;
        sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        sem_ci.pNext = nullptr;
        sem_ci.flags = 0;
        vkCreateSemaphore(m_device, &sem_ci, nullptr, &m_image_available_semaphore);
    }

    if (m_render_finished_semaphore == VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo sem_ci;
        sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        sem_ci.pNext = nullptr;
        sem_ci.flags = 0;
        vkCreateSemaphore(m_device, &sem_ci, nullptr, &m_render_finished_semaphore);
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
        required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return required_extensions;
}

std::vector<const char*> requiredInstanceLayers(bool debug) {
    std::vector<const char*> required_layers;

    if (debug) {
        required_layers.push_back("VK_LAYER_LUNARG_standard_validation");
    }

    return required_layers;
}

std::vector<const char*> requiredDeviceExtensions(bool debug) {
    std::vector<const char*> required_extensions;
    required_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    return required_extensions;
}

std::vector<const char*> requiredDeviceLayers(bool debug) {
    return std::vector<const char*>{};
}
