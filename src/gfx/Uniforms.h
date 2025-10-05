// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_UNIFORMS_H_
#define _VPLANET_GFX_UNIFORMS_H_

#include <vector>

#include "../vulkan.h"
#include "../VmaUsage.h"

#include "../glm_defines.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace gfx {
    class System;

    const int MAX_LIGHTS = 10;

    struct ViewProjectionTransform {
        glm::mat4x4 view;
        glm::mat4x4 projection;
    };

    struct LightInfo {
        glm::vec3 direction;
        uint32_t enabled;
    };

    class Uniforms {
    public:
        Uniforms(System *system);
        ~Uniforms();

        void init();
        void dispose();

        System* system();
        VkDescriptorPool descriptorPool() const;

    private:
        void initDescriptorPool();
        void cleanupDescriptorPool();

        System *m_system;
        VkDescriptorPool m_descriptor_pool;
    };

    class UniformSet {
    public:
        UniformSet(Uniforms *uniforms);
        virtual ~UniformSet();

        virtual void init();
        virtual void dispose();

        const std::vector<VkDescriptorSet>& descriptorSets() const;

    protected:
        virtual void initDescriptorSets() = 0;
        void cleanupDescriptorSets();

        Uniforms *m_uniforms;
        std::vector<VkDescriptorSet> m_descriptor_sets;
    };

    class SceneUniformSet : public UniformSet {
    public:
        SceneUniformSet(Uniforms *uniforms);
        virtual ~SceneUniformSet();

        virtual void init();
        virtual void dispose();

        static void initDescriptorSetLayout(System *gfx);
        static void cleanupDescriptorSetLayout(System *gfx);
        static VkDescriptorSetLayout descriptorSetLayout();

        void setTransforms(const ViewProjectionTransform &xform);
        void updateViewProjectionBuffer(uint32_t buffer_index);

        void enableLight(uint32_t index, const glm::vec3 &direction);
        void disableLight(uint32_t index);
        void updateLightListBuffer(uint32_t buffer_index);

    protected:
        void initUniformBuffers();
        void cleanupUniformBuffers();

        virtual void initDescriptorSets();

        static VkDescriptorSetLayout c_descriptor_set_layout;
        ViewProjectionTransform m_view_projection;
        LightInfo m_lights[MAX_LIGHTS];
        std::vector<VkBuffer> m_view_projection_buffers;
        std::vector<VmaAllocation> m_view_projection_buffer_allocations;
        std::vector<VkBuffer> m_light_list_buffers;
        std::vector<VmaAllocation> m_light_list_buffer_allocations;
    };

    class ModelUniformSet : public UniformSet {
    public:
        ModelUniformSet(Uniforms *uniforms);
        virtual ~ModelUniformSet();

        virtual void init();
        virtual void dispose();

        static void initDescriptorSetLayout(System *gfx);
        static void cleanupDescriptorSetLayout(System *gfx);
        static VkDescriptorSetLayout descriptorSetLayout();

        void setTransform(const glm::mat4x4 &model);
        void updateModelBuffer(uint32_t buffer_index);

    protected:
        void initUniformBuffers();
        void cleanupUniformBuffers();

        virtual void initDescriptorSets();

        static VkDescriptorSetLayout c_descriptor_set_layout;
        glm::mat4x4 m_model_transform;
        std::vector<VkBuffer> m_model_buffers;
        std::vector<VmaAllocation> m_model_buffer_allocations;
    };
}

#endif
