// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#ifndef _PLANET_MODELS_H_
#define _PLANET_MODELS_H_

#include <vector>

#include <glm/vec3.hpp>

struct PositionsAndElements {
    std::vector<glm::vec3> positions;
    std::vector<unsigned int> elements;
};

PositionsAndElements icosahedron();
PositionsAndElements icosphere(float radius, int refinements);

std::vector<glm::vec3> computeNormals(const PositionsAndElements &pne);

extern const double ICOSAHEDRON_VERTICES[12][3];
extern const unsigned int ICOSAHEDRON_VERTEX_COUNT;
extern const unsigned int ICOSAHEDRON_ELEMS[60];
extern const unsigned int ICOSAHEDRON_ELEM_COUNT;

#endif
