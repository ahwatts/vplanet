// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_TERRAIN_PIPELINE_H_
#define _VPLANET_GFX_TERRAIN_PIPELINE_H_

#include <vector>

#include "../glm.h"
#include "../vulkan.h"
#include "../VmaUsage.h"

#include "../Terrain.h"
#include "Pipeline.h"
#include "Resource.h"
#include "Uniforms.h"

namespace gfx {
    class Renderer;

    class TerrainPipeline : public Pipeline {
    public:
        TerrainPipeline();
        TerrainPipeline(Renderer *renderer);
        TerrainPipeline(const TerrainPipeline &other) = delete;
        TerrainPipeline(TerrainPipeline &&other) = default;

        virtual ~TerrainPipeline();

        TerrainPipeline &operator=(const TerrainPipeline &other) = delete;
        TerrainPipeline &operator=(TerrainPipeline &&other) = default;

        void setGeometry(const std::vector<TerrainVertex> &verts, const std::vector<uint32_t> &elems);
        void setTransform(const glm::mat4x4 &xform);
        void writeTransform(uint32_t buffer_index);

        void recordCommands(const vk::raii::CommandBuffer &cmd_buf, uint32_t frame_index);

    private:
        virtual void initPipeline();

        ModelUniformSet m_uniform_set;
        uint32_t m_num_indices;
        vk::raii::Buffer m_vertex_buffer, m_index_buffer;
        VmaAllocation m_vertex_buffer_allocation, m_index_buffer_allocation;
    };
}

#endif
