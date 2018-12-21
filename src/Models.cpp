// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

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