// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include <array>
#include <random>
#include <vector>

#include "vulkan.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

#include "Models.h"
#include "Ocean.h"

VkVertexInputBindingDescription OceanVertex::bindingDescription() {
    VkVertexInputBindingDescription desc;
    desc.binding = 0;
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    desc.stride = sizeof(OceanVertex);
    return desc;
}

std::array<VkVertexInputAttributeDescription, OceanVertex::NUM_ATTRIBUTES> OceanVertex::attributeDescription() {
    std::array<VkVertexInputAttributeDescription, OceanVertex::NUM_ATTRIBUTES> descs;

    descs[0].binding = 0;
    descs[0].location = 0;
    descs[0].offset = offsetof(OceanVertex, position);
    descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;

    descs[1].binding = 0;
    descs[1].location = 1;
    descs[1].offset = offsetof(OceanVertex, color);
    descs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;

    descs[2].binding = 0;
    descs[2].location = 2;
    descs[2].offset = offsetof(OceanVertex, normal);
    descs[2].format = VK_FORMAT_R32G32B32_SFLOAT;

    return descs;
}

Ocean::Ocean(float radius, int refinements)
    : m_vertices{},
      m_indices{}
{
    std::random_device seed;
    std::default_random_engine eng{seed()};
    std::uniform_real_distribution<float> dist{0.995, 1.005f};
    PositionsAndElements pne = icosphere(radius, refinements);

    for (size_t i = 0; i < pne.positions.size(); ++i) {
        float factor = dist(eng);
        pne.positions[i] *= factor;
    }

    std::vector<glm::vec3> normals = computeNormals(pne);

    m_indices = pne.elements;
    m_vertices.resize(pne.positions.size());
    for (size_t i = 0; i < pne.positions.size(); ++i) {
        m_vertices[i].position = pne.positions[i];
        m_vertices[i].color = { 0.2, 0.3, 0.6, 1.0 };
        m_vertices[i].normal = normals[i];
    }
}

Ocean::~Ocean() {}

const std::vector<OceanVertex>& Ocean::vertices() const {
    return m_vertices;
}

const std::vector<uint32_t>& Ocean::indices() const {
    return m_indices;
}
