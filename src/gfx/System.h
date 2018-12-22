// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_SYSTEM_H_
#define _VPLANET_GFX_SYSTEM_H_

#include <vector>
#include "../vulkan.h"

#include "../Terrain.h"
#include "Commands.h"
#include "DepthBuffer.h"
#include "Swapchain.h"
#include "TerrainRenderer.h"
#include "Uniforms.h"

namespace gfx {
    class System {
    public:
        System(GLFWwindow *window);
        ~System();

        void init(bool debug);
        void dispose();

        GLFWwindow* window() const;
        VkInstance instance() const;
        VkDevice device() const;
        VkPhysicalDevice physicalDevice() const;
        VkSurfaceKHR surface() const;
        uint32_t graphicsQueueFamily() const;
        uint32_t presentQueueFamily() const;

        const Commands& commands() const;
        const DepthBuffer& depthBuffer() const;
        const Swapchain& swapchain() const;
        const TerrainRenderer& terrainRenderer() const;
        const XformUniforms& transformUniforms() const;

        void setTerrainGeometry(const std::vector<TerrainVertex> &vertices, const std::vector<uint32_t> &elements);
        void setTransforms(const Transforms &xforms, uint32_t index);
        void recordCommandBuffers();
        uint32_t startFrame();
        void drawFrame(uint32_t image_index);
        void presentFrame(uint32_t image_index);
        void waitIdle();

        uint32_t chooseMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

        void createBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags mem_props,
            VkBuffer &buffer,
            VkDeviceMemory &memory);

        void copyBuffer(VkBuffer dst, VkBuffer src, VkDeviceSize size);

    private:
        void initInstance(bool debug);
        void cleanupInstance();

        void initDebugCallback();
        void cleanupDebugCallback();

        void initSurface();
        void cleanupSurface();

        void initDevice(bool debug);
        void cleanupDevice();

        void initSemaphores();
        void cleanupSemaphores();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT object_type,
            uint64_t object,
            size_t location,
            int32_t code,
            const char *layer_prefix,
            const char *message,
            void *user_data);
        VkBool32 debugCallback(
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT object_type,
            uint64_t object,
            size_t location,
            int32_t code,
            const char *layer_prefix,
            const char *message);

        GLFWwindow *m_window;

        VkInstance m_instance;
        VkDebugReportCallbackEXT m_debug_callback;
        VkSurfaceKHR m_surface;
        VkPhysicalDevice m_physical_device;
        VkDevice m_device;
        uint32_t m_graphics_queue_family, m_present_queue_family;
        VkSemaphore m_image_available_semaphore;
        VkSemaphore m_render_finished_semaphore;

        Commands m_commands;
        Swapchain m_swapchain;
        DepthBuffer m_depth_buffer;
        TerrainRenderer m_terrain_renderer;
        XformUniforms m_xform_uniforms;
    };
};

#endif
