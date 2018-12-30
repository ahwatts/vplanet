// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_RENDERER_H_
#define _VPLANET_GFX_RENDERER_H_

#include <vector>

#include "../vulkan.h"

#include "OceanPipeline.h"
#include "TerrainPipeline.h"

namespace gfx {
    class System;

    class Renderer {
    public:
        Renderer(System *system);
        ~Renderer();

        void init();
        void dispose();

        System* system();
        VkRenderPass renderPass() const;
        TerrainPipeline& terrainPipeline();
        OceanPipeline& oceanPipeline();

        void recordCommands(VkCommandBuffer cmd_buf, VkDescriptorSet xforms, uint32_t fb_index);

    private:
        void initRenderPass();
        void cleanupRenderPass();

        void initFramebuffers();
        void cleanupFramebuffers();

        System *m_system;
        VkRenderPass m_render_pass;
        std::vector<VkFramebuffer> m_framebuffers;

        OceanPipeline m_ocean_pipeline;
        TerrainPipeline m_terrain_pipeline;
    };
}

#endif
