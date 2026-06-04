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
        Renderer();
        Renderer(System *system);
        // Renderer(const Renderer &other) = delete;
        // Renderer(Renderer &&other) = default;

        // ~Renderer();

        // Renderer &operator=(const Renderer &other) = delete;
        // Renderer &operator=(Renderer &&other);

        System* system();
        const vk::raii::PipelineLayout &pipelineLayout() const;
        TerrainPipeline& terrainPipeline();
        OceanPipeline& oceanPipeline();

        void setViewProjectionTransform(const ViewProjectionTransform &xform);
        void writeViewProjectionTransform(uint32_t buffer_index);

        void enableLight(uint32_t index, const glm::vec3 &direction);
        void disableLight(uint32_t index);
        void writeLightList(uint32_t buffer_index);

        void recordCommands(const vk::raii::CommandBuffer &cmd_buf, uint32_t image_index, uint32_t frame_index);

    private:
        void initPipelineLayout();

        System *m_system;
        vk::raii::PipelineLayout m_pipeline_layout;

        SceneUniformSet m_uniform_set;
        OceanPipeline m_ocean_pipeline;
        TerrainPipeline m_terrain_pipeline;
    };
}

#endif
