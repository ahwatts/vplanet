// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <array>
#include <map>
#include <vector>

#include "glm.h"
#include "vulkan.h"

#include "Models.h"
#include "Noise.h"
#include "Terrain.h"

vk::VertexInputBindingDescription TerrainVertex::bindingDescription() {
    return vk::VertexInputBindingDescription{
        .binding = 0,
        .stride = sizeof(TerrainVertex),
        .inputRate = vk::VertexInputRate::eVertex,
    };
}

std::array<vk::VertexInputAttributeDescription, TerrainVertex::NUM_ATTRIBUTES> TerrainVertex::attributeDescription() {
    return std::array<vk::VertexInputAttributeDescription, TerrainVertex::NUM_ATTRIBUTES>{
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(TerrainVertex, position),
        },
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(TerrainVertex, normal),
        },
    };
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
