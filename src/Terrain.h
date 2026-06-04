// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_TERRAIN_H_
#define _VPLANET_TERRAIN_H_

#include <array>
#include <vector>

#include "vulkan.h"
#include "glm.h"

#include "Noise.h"

struct TerrainVertex {
    glm::vec3 position;
    glm::vec3 normal;
    static const int NUM_ATTRIBUTES = 2;

    static vk::VertexInputBindingDescription bindingDescription();
    static std::array<vk::VertexInputAttributeDescription, NUM_ATTRIBUTES> attributeDescription();
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
