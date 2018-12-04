// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_TERRAIN_H_
#define _VPLANET_TERRAIN_H_

#include <array>

#include "vulkan.h"

#include "glm_defines.h"
#include <glm/vec3.hpp>

struct TerrainVertex {
    glm::vec3 position;
    glm::vec3 normal;
    static const int NUM_ATTRIBUTES = 2;

    static VkVertexInputBindingDescription bindingDescription();
    static std::array<VkVertexInputAttributeDescription, NUM_ATTRIBUTES> attributeDescription();
};

#endif
