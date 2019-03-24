// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_TERRAIN_H_
#define _VPLANET_TERRAIN_H_

#include <array>
#include <vector>

#include "vulkan.h"

#include "glm_defines.h"
#include <glm/vec3.hpp>

#include "Noise.h"

struct TerrainVertex {
    glm::vec3 position;
    glm::vec3 normal;
    static const int NUM_ATTRIBUTES = 2;

    static VkVertexInputBindingDescription bindingDescription();
    static std::array<VkVertexInputAttributeDescription, NUM_ATTRIBUTES> attributeDescription();
};

class Terrain {
public:
    Terrain(float radius, int refinements, const NoiseFunction &noise);
    ~Terrain();

    const std::vector<TerrainVertex>& vertices() const;
    const std::vector<uint32_t>& elements() const;

private:
    std::vector<TerrainVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

#endif
