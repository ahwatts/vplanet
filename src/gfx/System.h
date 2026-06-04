// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_SYSTEM_H_
#define _VPLANET_GFX_SYSTEM_H_

#include <vector>

#include "../vulkan.h"
#include "../VmaUsage.h"

#include "../Terrain.h"
#include "Commands.h"
#include "DepthBuffer.h"
#include "Renderer.h"
#include "Resource.h"
#include "Swapchain.h"
#include "Uniforms.h"

namespace gfx {
    class System {
    public:
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
        
        System(GLFWwindow *window, bool debug);
        ~System();

        GLFWwindow* window() const;
        const vk::raii::Instance &instance() const;
        const vk::raii::Device &device() const;
        const vk::raii::PhysicalDevice &physicalDevice() const;
        const vk::raii::SurfaceKHR &surface() const;
        uint32_t graphicsQueueFamily() const;
        uint32_t presentQueueFamily() const;
        uint32_t numFrames() const;

        VmaAllocator allocator() const;

        const Commands& commands() const;
        const DepthBuffer& depthBuffer() const;
        const Swapchain& swapchain() const;
        const Renderer& renderer() const;
        Uniforms& uniforms();

        void setTerrainGeometry(const std::vector<TerrainVertex> &vertices, const std::vector<uint32_t> &indices);
        void setTerrainTransform(const glm::mat4x4 &xform);
        void writeTerrainTransform();
        void writeTerrainTransform(uint32_t frame_index);

        void setOceanGeometry(const std::vector<OceanVertex> &vertices, const std::vector<uint32_t> &indices);
        void setOceanTransform(const glm::mat4x4 &xform);
        void writeOceanTransform();
        void writeOceanTransform(uint32_t frame_index);

        void setViewProjectionTransform(const ViewProjectionTransform &xform);
        void writeViewProjectionTransform();
        void writeViewProjectionTransform(uint32_t frame_index);

        void enableLight(uint32_t index, const glm::vec3 &direction);
        void disableLight(uint32_t index);
        void writeLightList();
        void writeLightList(uint32_t frame_index);

        // void recordCommandBuffers();
        uint32_t startFrame();
        void drawFrame(uint32_t image_index);
        void presentFrame(uint32_t image_index);
        void waitIdle();

        uint32_t chooseMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

        std::pair<vk::raii::Buffer, VmaAllocation> createBuffer(
            vk::DeviceSize size, 
            vk::BufferUsageFlags usage,
            VmaAllocationCreateFlags allocation_flags
        );
        std::pair<vk::raii::Buffer, VmaAllocation> createBufferWithData(
            const void *data, 
            size_t size, 
            vk::BufferUsageFlags usage,
            VmaAllocationCreateFlags allocation_flags
        );
        void copyBuffer(const vk::raii::Buffer &dst, const vk::raii::Buffer &src, vk::DeviceSize size);

    private:
        void initInstance();
        void initDebugCallback();
        void initSurface();
        void initDevice();
        void initSynchronizationObjects();

        void initAllocator();
        void cleanupAllocator();

        static VKAPI_ATTR vk::Bool32 debugCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
            vk::DebugUtilsMessageTypeFlagsEXT message_types,
            const vk::DebugUtilsMessengerCallbackDataEXT *data,
            void *user_data
        );
        vk::Bool32 debugCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
            vk::DebugUtilsMessageTypeFlagsEXT message_types,
            const vk::DebugUtilsMessengerCallbackDataEXT *data
        );

        static VKAPI_ATTR void VKAPI_CALL memoryAllocationCallback(
            VmaAllocator allocator,
            uint32_t memoryType,
            VkDeviceMemory memory,
            VkDeviceSize size,
            void *user_data);
        void memoryAllocationCallback(
            VmaAllocator allocator,
            uint32_t memoryType,
            VkDeviceMemory memory,
            VkDeviceSize size);
        static VKAPI_ATTR void VKAPI_CALL memoryFreeCallback(
            VmaAllocator allocator,
            uint32_t memoryType,
            VkDeviceMemory memory,
            VkDeviceSize size,
            void *user_data);
        void memoryFreeCallback(
            VmaAllocator allocator,
            uint32_t memoryType,
            VkDeviceMemory memory,
            VkDeviceSize size);

        GLFWwindow *m_window;
        bool m_debug;
        uint32_t m_frame_index;

        vk::raii::Context m_context;
        vk::raii::Instance m_instance;
        vk::raii::DebugUtilsMessengerEXT m_debug_messenger;
        vk::raii::SurfaceKHR m_surface;
        vk::raii::PhysicalDevice m_physical_device;
        vk::raii::Device m_device;
        uint32_t m_graphics_queue_family, m_present_queue_family;
        std::vector<vk::raii::Semaphore> m_present_complete_semaphores;
        std::vector<vk::raii::Semaphore> m_render_finished_semaphores;
        std::vector<vk::raii::Fence> m_draw_fences;

        VmaAllocator m_allocator;

        Commands m_commands;
        Swapchain m_swapchain;
        DepthBuffer m_depth_buffer;
        Renderer m_renderer;
        Uniforms m_uniforms;
    };
};

#endif
