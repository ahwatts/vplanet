// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <array>
#include <map>
#include <vector>
#include "vulkan.h"

#include "glm_defines.h"
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

#include "Models.h"
#include "Noise.h"
#include "Terrain.h"

VkVertexInputBindingDescription TerrainVertex::bindingDescription() {
    VkVertexInputBindingDescription desc;
    desc.binding = 0;
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    desc.stride = sizeof(TerrainVertex);
    return desc;
}

std::array<VkVertexInputAttributeDescription, TerrainVertex::NUM_ATTRIBUTES> TerrainVertex::attributeDescription() {
    std::array<VkVertexInputAttributeDescription, TerrainVertex::NUM_ATTRIBUTES> descs;

    descs[0].binding = 0;
    descs[0].location = 0;
    descs[0].offset = offsetof(TerrainVertex, position);
    descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;

    descs[1].binding = 0;
    descs[1].location = 1;
    descs[1].offset = offsetof(TerrainVertex, normal);
    descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;

    return descs;
}

Terrain::Terrain(float radius, int refinements, const NoiseFunction &noise)
    : m_vertices{},
      m_indices{}
{
    PositionsAndElements pne = icosphere(radius, refinements);

    for (size_t i = 0; i < pne.positions.size(); ++i) {
        glm::vec3 &pos = pne.positions[i];
        double n = noise(pos.x, pos.y, pos.z);
        pos *= n/8.0 + 1.0;
    }

    std::vector<glm::vec3> normals = computeNormals(pne);

    m_indices = pne.elements;
    m_vertices.resize(pne.positions.size());
    for (size_t i = 0; i < pne.positions.size(); ++i) {
        m_vertices[i].position = pne.positions[i];
        m_vertices[i].normal = normals[i];
    }
}

Terrain::~Terrain() {}

const std::vector<TerrainVertex>& Terrain::vertices() const {
    return m_vertices;
}

const std::vector<uint32_t>& Terrain::elements() const {
    return m_indices;
}
