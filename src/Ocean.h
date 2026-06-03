// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#ifndef _VPLANET_OCEAN_H_
#define _VPLANET_OCEAN_H_

#include <array>
#include <vector>

#include "glm.h"
#include "vulkan.h"

struct OceanVertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec3 normal;
    static const int NUM_ATTRIBUTES = 3;

    static vk::VertexInputBindingDescription bindingDescription();
    static std::array<vk::VertexInputAttributeDescription, NUM_ATTRIBUTES> attributeDescription();
};

class Ocean {
public:
    Ocean(float radius, int refinements);
    ~Ocean();

    const std::vector<OceanVertex>& vertices() const;
    const std::vector<uint32_t>& indices() const;

private:
    std::vector<OceanVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

#endif
