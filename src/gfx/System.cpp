// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../vulkan.h"

#include "Swapchain.h"
#include "System.h"

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
      m_swapchain{this},
      m_depth_buffer{this},
      m_terrain_renderer{this}
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
    m_swapchain.init();
    m_depth_buffer.init();
    m_terrain_renderer.init();
}

void gfx::System::dispose() {
    m_terrain_renderer.dispose();
    m_depth_buffer.dispose();
    m_swapchain.dispose();
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

const gfx::Swapchain& gfx::System::swapchain() const {
    return m_swapchain;
}

const gfx::DepthBuffer& gfx::System::depthBuffer() const {
    return m_depth_buffer;
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
