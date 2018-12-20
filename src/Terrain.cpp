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
    loadIcosphere(radius, refinements);
    varyPositions(noise);
    computeNormals();
}

Terrain::~Terrain() {}

const std::vector<TerrainVertex>& Terrain::vertices() const {
    return m_vertices;
}

const std::vector<uint32_t>& Terrain::elements() const {
    return m_indices;
}

void Terrain::loadIcosphere(float radius, int refinements) {
    const PositionsAndElements pne = icosphere(radius, refinements);
    m_vertices.resize(pne.positions.size());

    m_indices = pne.elements;
    for (size_t i = 0; i < pne.positions.size(); ++i) {
        m_vertices[i].position = pne.positions[i];
        m_vertices[i].normal = glm::normalize(pne.positions[i]);
    }
}

void Terrain::varyPositions(const NoiseFunction &noise) {
    for (size_t i = 0; i < m_vertices.size(); ++i) {
        glm::vec3 &pos = m_vertices[i].position;
        double n = noise(pos.x, pos.y, pos.z);
        pos *= n/8.0 + 1.0;
    }
}

void Terrain::computeNormals() {
    // Build an adjacency map of vertices to triangles (as base
    // element indices).
    std::map<uint32_t, std::vector<uint32_t> > adj_map;
    for (uint32_t tid = 0; tid < m_indices.size(); tid += 3) {
        adj_map[m_indices[tid+0]].push_back(tid);
        adj_map[m_indices[tid+1]].push_back(tid);
        adj_map[m_indices[tid+2]].push_back(tid);
    }

    // Compute the normal for each vertex.
    for (uint32_t vid = 0; vid < m_vertices.size(); ++vid) {
        glm::vec3 &vp = m_vertices[vid].position;

        // Compute the vertex normal as a weighted average of the
        // facet normals for the triangles adjacent to this vertex.
        glm::vec3 vertex_normal{0.0f, 0.0f, 0.0f};

        for (auto tid : adj_map[vid]) {
            unsigned int vid1 = m_indices[tid+0];
            unsigned int vid2 = m_indices[tid+1];
            unsigned int vid3 = m_indices[tid+2];
            const glm::vec3 &v1 = m_vertices[vid1].position;
            const glm::vec3 &v2 = m_vertices[vid2].position;
            const glm::vec3 &v3 = m_vertices[vid3].position;
            glm::vec3 cross = glm::cross(v2 - v1, v3 - v1);
            glm::vec3 face_normal = glm::normalize(cross);

            // Weight by the area (the mangitude of the cross product
            // is twice the area of the triangle). The extra factor of
            // 2 is unimportant, since we're normalizing the result.
            float area = glm::length(cross);

            // Also weight by the angle of the triangle at this
            // vertex.
            glm::vec3 s1, s2;
            if (vid == vid1) {
                s1 = v1 - v2;
                s2 = v1 - v3;
            } else if (vid == vid2) {
                s1 = v2 - v1;
                s2 = v2 - v3;
            } else if (vid == vid3) {
                s1 = v3 - v1;
                s2 = v3 - v2;
            }
            float angle = std::acos(glm::dot(s1, s2) / glm::length(s1) / glm::length(s2));

            vertex_normal += face_normal * area * angle;
        }

        m_vertices[vid].normal = glm::normalize(vertex_normal);
    }
}
