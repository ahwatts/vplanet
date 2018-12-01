// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include "../vulkan.h"

#include "System.h"

struct ChosenDeviceInfo {
    VkPhysicalDevice device;
    uint32_t graphics_queue_family;
    uint32_t present_queue_family;
};

ChosenDeviceInfo choosePhysicalDevice(const std::vector<VkPhysicalDevice> &devices, VkSurfaceKHR surface);

gfx::System::System(GLFWwindow *window)
    : m_window{window},
      m_instance{VK_NULL_HANDLE},
      m_debug_callback{VK_NULL_HANDLE},
      m_surface{VK_NULL_HANDLE},
      m_device{VK_NULL_HANDLE},
      m_physical_device{VK_NULL_HANDLE}
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
    initDevice();
}

void gfx::System::dispose() {
    cleanupDevice();
    cleanupSurface();
    cleanupDebugCallback();
    cleanupInstance();
}

void gfx::System::initInstance(bool debug) {
    if (m_instance != VK_NULL_HANDLE) {
        return;
    }

    std::vector<const char*> wanted_extensions;

    uint32_t glfw_extension_count;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    for (uint32_t i = 0; i < glfw_extension_count; ++i) {
        wanted_extensions.push_back(glfw_extensions[i]);
    }

    if (debug) {
        wanted_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

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

    std::vector<const char *> wanted_layers;
    if (debug) {
        wanted_layers.push_back("VK_LAYER_LUNARG_standard_validation");
    }

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
    app_info.applicationVersion = 0;
    app_info.pEngineName = nullptr;
    app_info.engineVersion = 0;

    VkInstanceCreateInfo inst_ci{};
    inst_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_ci.pNext = nullptr;
    inst_ci.flags = 0;
    inst_ci.pApplicationInfo = &app_info;
    inst_ci.enabledLayerCount = static_cast<uint32_t>(wanted_layers.size());
    inst_ci.ppEnabledLayerNames = wanted_layers.data();
    inst_ci.enabledExtensionCount = static_cast<uint32_t>(wanted_extensions.size());
    inst_ci.ppEnabledExtensionNames = wanted_extensions.data();

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
        std::cerr << "Vulkan error: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        std::cerr << "Vulkan warning: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        std::cerr << "Vulkan performance warning: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        std::cerr << "Vulkan info: " << layer_prefix << ": " << message;
    } else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        std::cerr << "Vulkan debug: " << layer_prefix << ": " << message;
    } else {
        std::cerr << "Vulkan unknown level: " << layer_prefix << ": " << message;
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

void gfx::System::initDevice() {
    if (m_device != VK_NULL_HANDLE) {
        return;
    }

    uint32_t num_devices;
    vkEnumeratePhysicalDevices(m_instance, &num_devices, nullptr);
    std::vector<VkPhysicalDevice> devices{num_devices};
    vkEnumeratePhysicalDevices(m_instance, &num_devices, devices.data());
}

void gfx::System::cleanupDevice() {
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
}

ChosenDeviceInfo choosePhysicalDevice(const std::vector<VkPhysicalDevice> &devices, VkSurfaceKHR surface) {
    const uint32_t MAX_INT = std::numeric_limits<uint32_t>::max();

    for (auto &device : devices) {
        // Does it have the properties we want?
        // VkPhysicalDeviceProperties properties;
        // vkGetPhysicalDeviceProperties(device, &properties);

        // Does it support the features we want?
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);
        if (features.samplerAnisotropy != VK_TRUE) {
            continue;
        }

        // Do we have appropriate queue families for graphics / presentation?
        uint32_t num_queue_families, graphics_queue = MAX_INT, present_queue = MAX_INT;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, queue_families.data());
        for (uint32_t id = 0; id < queue_families.size(); ++id) {
            if (graphics_queue == MAX_INT && queue_families[id].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_queue = id;
            }

            if (present_queue == MAX_INT) {
                VkBool32 supports_present;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, id, surface, &supports_present);
                present_queue = id;
            }

            if (graphics_queue < MAX_INT && present_queue < MAX_INT) {
                break;
            }
        }
        if (graphics_queue == MAX_INT && present_queue == MAX_INT) {
            continue;
        }

        // Are the extensions / layers we want supported?
        // Are swapchains supported, and are there surface formats and present modes we can use?
    }
}
