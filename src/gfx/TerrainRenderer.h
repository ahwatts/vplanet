// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_TERRAIN_RENDERER_H_
#define _VPLANET_GFX_TERRAIN_RENDERER_H_

#include "../vulkan.h"
#include "Resource.h"

namespace gfx {
    class System;

    class TerrainRenderer {
    public:
        TerrainRenderer(System *system);
        ~TerrainRenderer();

        void init();
        void dispose();

        VkRenderPass renderPass() const;

        void recordCommands(
            VkCommandBuffer &cmd_buf,
            VkBuffer &vertices,
            VkBuffer &indices,
            uint32_t num_indices,
            VkDescriptorSet &xforms,
            VkFramebuffer &dst);

    private:
        void initShaderModules();
        void cleanupShaderModules();

        void initRenderPass();
        void cleanupRenderPass();

        void initDescriptorSetLayout();
        void cleanupDescriptorSetLayout();

        void initPipelineLayout();
        void cleanupPipelineLayout();

        void initPipeline();
        void cleanupPipeline();

        VkShaderModule createShaderModule(const Resource &rsrc);

        System *m_system;
        VkShaderModule m_vertex_shader, m_fragment_shader;
        VkRenderPass m_render_pass;
        VkDescriptorSetLayout m_descriptor_set_layout;
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_pipeline;
    };
}

#endif
