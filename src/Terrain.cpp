// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <array>
#include "vulkan.h"
#include "glm_defines.h"
#include <glm/vec3.hpp>
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
