// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_TERRAIN_PIPELINE_H_
#define _VPLANET_GFX_TERRAIN_PIPELINE_H_

#include <vector>

#include "../vulkan.h"

#include "../Terrain.h"
#include "Resource.h"

namespace gfx {
    class Renderer;

    class TerrainPipeline {
    public:
        TerrainPipeline(Renderer *renderer);
        ~TerrainPipeline();

        void init();
        void dispose();

        void setGeometry(const std::vector<TerrainVertex> &verts, const std::vector<uint32_t> &elems);
        void recordCommands(VkCommandBuffer cmd_buf, VkDescriptorSet xforms);

    private:
        void initShaderModules();
        void cleanupShaderModules();

        void initPipelineLayout();
        void cleanupPipelineLayout();

        void initPipeline();
        void cleanupPipeline();

        template<typename T>
        void initGeometryBuffer(VkBuffer &dst_buffer, VkDeviceMemory &dst_memory, const std::vector<T> &data, VkBufferUsageFlags buffer_type);
        void cleanupGeometryBuffers();

        VkShaderModule createShaderModule(const Resource &rsrc);

        Renderer *m_renderer;
        VkShaderModule m_vertex_shader, m_fragment_shader;
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_pipeline;
        uint32_t m_num_indices;
        VkBuffer m_vertex_buffer, m_index_buffer;
        VkDeviceMemory m_vertex_buffer_memory, m_index_buffer_memory;
    };
}

#endif
