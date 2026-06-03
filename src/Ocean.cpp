// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include <array>
#include <random>
#include <vector>

#include "vulkan.h"

#include "glm.h"

#include "Models.h"
#include "Ocean.h"

vk::VertexInputBindingDescription OceanVertex::bindingDescription() {
    return vk::VertexInputBindingDescription{
        .binding = 0,
        .stride = sizeof(OceanVertex),
        .inputRate = vk::VertexInputRate::eVertex,
    };
}

std::array<vk::VertexInputAttributeDescription, OceanVertex::NUM_ATTRIBUTES> OceanVertex::attributeDescription() {
    return std::array<vk::VertexInputAttributeDescription, OceanVertex::NUM_ATTRIBUTES>{
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(OceanVertex, position),
        },
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32A32Sfloat,
            .offset = offsetof(OceanVertex, color),
        },
        vk::VertexInputAttributeDescription{
            .location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(OceanVertex, normal),
        },
    };
}

Ocean::Ocean(float radius, int refinements)
    : m_vertices{},
      m_indices{}
{
    std::random_device seed;
    std::default_random_engine eng{seed()};
    std::uniform_real_distribution<float> dist{0.995f, 1.005f};
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
