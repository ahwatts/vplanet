// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_UNIFORMS_H_
#define _VPLANET_GFX_UNIFORMS_H_

#include <vector>

#include "../vulkan.h"

#include "../glm_defines.h"
#include <glm/mat4x4.hpp>

namespace gfx {
    class System;

    struct Transforms {
        glm::mat4x4 model;
        glm::mat4x4 view;
        glm::mat4x4 projection;
    };

    class XformUniforms {
    public:
        XformUniforms(System *system);
        ~XformUniforms();

        void init();
        void dispose();

        VkDescriptorSetLayout descriptorSetLayout() const;
        const std::vector<VkDescriptorSet>& descriptorSets() const;

        void setTransforms(const Transforms &xforms, uint32_t buffer_index);

    private:
        void initDescriptorSetLayout();
        void cleanupDescriptorSetLayout();

        void initDescriptorPool();
        void cleanupDescriptorPool();

        void initUniformBuffers();
        void cleanupUniformBuffers();

        void initDescriptorSets();
        void cleanupDescriptorSets();

        System *m_system;

        VkDescriptorSetLayout m_descriptor_set_layout;
        VkDescriptorPool m_descriptor_pool;
        std::vector<VkDescriptorSet> m_descriptor_sets;

        std::vector<VkBuffer> m_buffers;
        std::vector<VkDeviceMemory> m_memories;
    };
}

#endif
