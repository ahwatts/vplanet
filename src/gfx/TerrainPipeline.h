// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_TERRAIN_PIPELINE_H_
#define _VPLANET_GFX_TERRAIN_PIPELINE_H_

#include <vector>

#include "../vulkan.h"

#include "../Terrain.h"
#include "Pipeline.h"
#include "Resource.h"
#include "Uniforms.h"

namespace gfx {
    class Renderer;

    class TerrainPipeline : public Pipeline {
    public:
        TerrainPipeline(Renderer *renderer);
        virtual ~TerrainPipeline();

        void init();
        void dispose();

        void setGeometry(const std::vector<TerrainVertex> &verts, const std::vector<uint32_t> &elems);
        void recordCommands(VkCommandBuffer cmd_buf, VkDescriptorSet xforms);

    private:
        void initShaderModules();
        void cleanupShaderModules();

        virtual void initPipelineLayout();
        virtual void initPipeline();

        void cleanupGeometryBuffers();

        ModelUniformSet m_uniforms;
        VkShaderModule m_vertex_shader, m_fragment_shader;
        uint32_t m_num_indices;
        VkBuffer m_vertex_buffer, m_index_buffer;
        VkDeviceMemory m_vertex_buffer_memory, m_index_buffer_memory;
    };
}

#endif
