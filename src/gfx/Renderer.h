// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_RENDERER_H_
#define _VPLANET_GFX_RENDERER_H_

#include <vector>

#include "../vulkan.h"

#include "OceanPipeline.h"
#include "TerrainPipeline.h"
#include "Uniforms.h"

namespace gfx {
    class System;

    class Renderer {
    public:
        Renderer(System *system);
        ~Renderer();

        void init();
        void dispose();

        System* system();
        VkPipelineLayout pipelineLayout() const;
        VkRenderPass renderPass() const;
        TerrainPipeline& terrainPipeline();
        OceanPipeline& oceanPipeline();

        void setViewProjectionTransform(const ViewProjectionTransform &xform);
        void writeViewProjectionTransform(uint32_t buffer_index);

        void enableLight(uint32_t index, const glm::vec3 &direction);
        void disableLight(uint32_t index);
        void writeLightList(uint32_t buffer_index);

        void recordCommands(VkCommandBuffer cmd_buf, uint32_t fb_index);

    private:
        void initPipelineLayout();
        void cleanupPipelineLayout();

        void initRenderPass();
        void cleanupRenderPass();

        void initFramebuffers();
        void cleanupFramebuffers();

        System *m_system;
        VkPipelineLayout m_pipeline_layout;
        VkRenderPass m_render_pass;
        std::vector<VkFramebuffer> m_framebuffers;

        SceneUniformSet m_uniforms;
        OceanPipeline m_ocean_pipeline;
        TerrainPipeline m_terrain_pipeline;
    };
}

#endif
