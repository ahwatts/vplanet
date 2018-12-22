// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_TERRAIN_RENDERER_H_
#define _VPLANET_GFX_TERRAIN_RENDERER_H_

#include <vector>
#include "../vulkan.h"
#include "DepthBuffer.h"
#include "Resource.h"
#include "System.h"

namespace gfx {
    class TerrainRenderer {
    public:
        TerrainRenderer(System *system);
        ~TerrainRenderer();

        void init(const std::vector<VkImageView> &color_buffers, const DepthBuffer &depth_buffer);
        void dispose();

        VkRenderPass renderPass() const;

        void setGeometry(const std::vector<TerrainVertex> &verts, const std::vector<uint32_t> &elems);
        void recordCommands(const VkCommandBuffer &cmd_buf, const VkDescriptorSet &xforms, uint32_t framebuffer_index);

    private:
        void initShaderModules();
        void cleanupShaderModules();

        void initRenderPass();
        void cleanupRenderPass();

        void initPipelineLayout();
        void cleanupPipelineLayout();

        void initPipeline();
        void cleanupPipeline();

        void initFramebuffers(const std::vector<VkImageView> &color_buffers, const DepthBuffer &depth_buffer);
        void cleanupFramebuffers();

        template<typename T>
        void initGeometryBuffer(VkBuffer &dst_buffer, VkDeviceMemory &dst_memory, const std::vector<T> &data, VkBufferUsageFlags buffer_type);
        void cleanupGeometryBuffers();

        VkShaderModule createShaderModule(const Resource &rsrc);

        System *m_system;
        VkShaderModule m_vertex_shader, m_fragment_shader;
        VkRenderPass m_render_pass;
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_pipeline;
        std::vector<VkFramebuffer> m_framebuffers;
        uint32_t m_num_indices;
        VkBuffer m_vertex_buffer, m_index_buffer;
        VkDeviceMemory m_vertex_buffer_memory, m_index_buffer_memory;
    };
}

#endif
