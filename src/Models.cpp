// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

#include "glm_defines.h"
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>

#include "Models.h"

PositionsAndElements icosahedron() {
    PositionsAndElements rv;
    for (unsigned int i = 0; i < ICOSAHEDRON_VERTEX_COUNT; ++i) {
        rv.positions.push_back(glm::make_vec3(ICOSAHEDRON_VERTICES[i]));
    }
    for (unsigned int i = 0; i < ICOSAHEDRON_ELEM_COUNT; ++i) {
        rv.elements = std::vector<unsigned int>{ICOSAHEDRON_ELEMS, ICOSAHEDRON_ELEMS + ICOSAHEDRON_ELEM_COUNT};
    }
    return rv;
}

std::pair<unsigned int, unsigned int> edgeKey(unsigned int e1, unsigned int e2) {
    return std::pair<unsigned int, unsigned int>{std::min(e1, e2), std::max(e1, e2)};
}

PositionsAndElements refine(const PositionsAndElements &old_vertices) {
    PositionsAndElements new_vertices;
    std::map<std::pair<unsigned int, unsigned int>, unsigned int> edge_map;
    new_vertices.positions = old_vertices.positions;

    for (unsigned int i = 0; i < old_vertices.elements.size(); i += 3) {
        unsigned int
            e1 = old_vertices.elements[i+0],
            e2 = old_vertices.elements[i+1],
            e3 = old_vertices.elements[i+2];

        const glm::vec3
            &p1 = old_vertices.positions[e1],
            &p2 = old_vertices.positions[e2],
            &p3 = old_vertices.positions[e3];

        auto e12 = edge_map.find(edgeKey(e1, e2));
        if (e12 == edge_map.end()) {
            e12 = edge_map.insert({ edgeKey(e1, e2), static_cast<unsigned int>(new_vertices.positions.size()) }).first;
            new_vertices.positions.push_back((p1 + p2) * 0.5f);
        }

        auto e23 = edge_map.find(edgeKey(e2, e3));
        if (e23 == edge_map.end()) {
            e23 = edge_map.insert({ edgeKey(e2, e3), static_cast<unsigned int>(new_vertices.positions.size()) }).first;
            new_vertices.positions.push_back((p2 + p3) * 0.5f);
        }

        auto e13 = edge_map.find(edgeKey(e1, e3));
        if (e13 == edge_map.end()) {
            e13 = edge_map.insert({ edgeKey(e1, e3), static_cast<unsigned int>(new_vertices.positions.size()) }).first;
            new_vertices.positions.push_back((p1 + p3) * 0.5f);
        }

        new_vertices.elements.push_back(e1);
        new_vertices.elements.push_back(e12->second);
        new_vertices.elements.push_back(e13->second);

        new_vertices.elements.push_back(e2);
        new_vertices.elements.push_back(e23->second);
        new_vertices.elements.push_back(e12->second);

        new_vertices.elements.push_back(e3);
        new_vertices.elements.push_back(e13->second);
        new_vertices.elements.push_back(e23->second);

        new_vertices.elements.push_back(e12->second);
        new_vertices.elements.push_back(e23->second);
        new_vertices.elements.push_back(e13->second);
    }

    return new_vertices;
}


PositionsAndElements icosphere(float radius, int refinements) {
    PositionsAndElements rv = icosahedron();
    for (int i = 0; i < refinements; ++i) {
        rv = refine(rv);
    }
    for (auto &pos : rv.positions) {
        pos = glm::normalize(pos) * radius;
    }
    return rv;
}

std::vector<glm::vec3> computeNormals(const PositionsAndElements &pne) {
    std::vector<glm::vec3> normals{pne.positions.size()};

    // Build an adjacency map of vertices to triangles (as base
    // element indices).
    std::map<uint32_t, std::vector<uint32_t> > adj_map;
    for (uint32_t tid = 0; tid < pne.elements.size(); tid += 3) {
        adj_map[pne.elements[tid+0]].push_back(tid);
        adj_map[pne.elements[tid+1]].push_back(tid);
        adj_map[pne.elements[tid+2]].push_back(tid);
    }

    // Compute the normal for each vertex.
    for (uint32_t vid = 0; vid < pne.positions.size(); ++vid) {
        const glm::vec3 &vp = pne.positions[vid];

        // Compute the vertex normal as a weighted average of the
        // facet normals for the triangles adjacent to this vertex.
        glm::vec3 vertex_normal{0.0f, 0.0f, 0.0f};

        for (auto tid : adj_map[vid]) {
            unsigned int vid1 = pne.elements[tid+0];
            unsigned int vid2 = pne.elements[tid+1];
            unsigned int vid3 = pne.elements[tid+2];
            const glm::vec3 &v1 = pne.positions[vid1];
            const glm::vec3 &v2 = pne.positions[vid2];
            const glm::vec3 &v3 = pne.positions[vid3];
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

        normals[vid] = glm::normalize(vertex_normal);
    }

    return normals;
}

extern const double PHI = (1.0 + std::sqrt(5.0)) / 2.0;

extern const double ICOSAHEDRON_VERTICES[12][3] = {
    {  1.0,  PHI,  0.0 }, // 0
    { -1.0,  PHI,  0.0 }, // 1
    {  1.0, -PHI,  0.0 }, // 2
    { -1.0, -PHI,  0.0 }, // 3
    {  PHI,  0.0,  1.0 }, // 4
    {  PHI,  0.0, -1.0 }, // 5
    { -PHI,  0.0,  1.0 }, // 6
    { -PHI,  0.0, -1.0 }, // 7
    {  0.0,  1.0,  PHI }, // 8
    {  0.0, -1.0,  PHI }, // 9
    {  0.0,  1.0, -PHI }, // 10
    {  0.0, -1.0, -PHI }, // 11
};

extern const unsigned int ICOSAHEDRON_VERTEX_COUNT = sizeof(ICOSAHEDRON_VERTICES) / sizeof(ICOSAHEDRON_VERTICES[0]);

extern const unsigned int ICOSAHEDRON_ELEMS[60] = {
    1, 7, 6,
    1, 6, 8,
    1, 8, 0,
    1, 0, 10,
    1, 10, 7,
    7, 3, 6,
    6, 3, 9,
    6, 9, 8,
    8, 9, 4,
    8, 4, 0,
    0, 4, 5,
    0, 5, 10,
    10, 5, 11,
    10, 11, 7,
    7, 11, 3,
    3, 2, 9,
    9, 2, 4,
    4, 2, 5,
    5, 2, 11,
    11, 2, 3,
};

extern const unsigned int ICOSAHEDRON_ELEM_COUNT = sizeof(ICOSAHEDRON_ELEMS) / sizeof(ICOSAHEDRON_ELEMS[0]);
