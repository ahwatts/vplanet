// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_OCEAN_PIPELINE_H_
#define _VPLANET_GFX_OCEAN_PIPELINE_H_

#include "../vulkan.h"

#include "../Ocean.h"
#include "Pipeline.h"

namespace gfx {
    class Renderer;

    class OceanPipeline : public Pipeline {
    public:
        OceanPipeline(Renderer *renderer);
        ~OceanPipeline();

        void init();
        void dispose();

        void setGeometry(const std::vector<OceanVertex> &verts, const std::vector<uint32_t> &elems);
        void recordCommands(VkCommandBuffer cmd_buf, VkDescriptorSet xforms);

    private:
        void initShaderModules();
        void cleanupShaderModules();

        virtual void initPipelineLayout();
        virtual void initPipeline();

        void cleanupGeometryBuffers();

        VkShaderModule m_vertex_shader, m_fragment_shader;
        uint32_t m_num_indices;
        VkBuffer m_vertex_buffer, m_index_buffer;
        VkDeviceMemory m_vertex_buffer_memory, m_index_buffer_memory;
    };
}

#endif