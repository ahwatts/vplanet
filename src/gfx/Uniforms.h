// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_UNIFORMS_H_
#define _VPLANET_GFX_UNIFORMS_H_

#include <vector>

#include "../glm.h"
#include "../vulkan.h"
#include "../VmaUsage.h"

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
        Uniforms();
        Uniforms(System *system, uint32_t num_frames);
        Uniforms(const Uniforms &other) = delete;
        Uniforms(Uniforms &&other) = default;

        ~Uniforms();

        Uniforms &operator=(const Uniforms &other) = delete;
        Uniforms &operator=(Uniforms &&other) = default;

        System* system();
        const vk::raii::DescriptorPool &descriptorPool() const;
        uint32_t numFrames() const;

        vk::raii::DescriptorSetLayout *registerDescriptorSetLayout(vk::raii::DescriptorSetLayout &&layout);

    private:
        void initDescriptorPool();

        System *m_system;
        vk::raii::DescriptorPool m_descriptor_pool;
        std::vector<vk::raii::DescriptorSetLayout> m_descriptor_set_layouts;
        uint32_t m_num_frames;
    };

    class UniformSet {
    public:
        UniformSet();
        UniformSet(Uniforms *uniforms);
        UniformSet(const UniformSet &other) = delete;
        UniformSet(UniformSet &&other);

        virtual ~UniformSet();

        UniformSet &operator=(const UniformSet &other) = delete;
        UniformSet &operator=(UniformSet &&other);

        const std::vector<vk::raii::DescriptorSet> &descriptorSets() const;

    protected:
        virtual void initDescriptorSets() = 0;

        Uniforms *m_uniforms;
        std::vector<vk::raii::DescriptorSet> m_descriptor_sets;
    };

    class SceneUniformSet : public UniformSet {
    public:
        SceneUniformSet();
        SceneUniformSet(Uniforms *uniforms);
        SceneUniformSet(const SceneUniformSet &other) = delete;
        SceneUniformSet(SceneUniformSet &&other);

        virtual ~SceneUniformSet();

        SceneUniformSet &operator=(const SceneUniformSet &other) = delete;
        SceneUniformSet &operator=(SceneUniformSet &&other);

        static void initDescriptorSetLayout(System *gfx, Uniforms *owner);
        static const vk::raii::DescriptorSetLayout &descriptorSetLayout();

        void setTransforms(const ViewProjectionTransform &xform);
        void updateViewProjectionBuffer(uint32_t buffer_index);

        void enableLight(uint32_t index, const glm::vec3 &direction);
        void disableLight(uint32_t index);
        void updateLightListBuffer(uint32_t buffer_index);

    protected:
        friend class Uniforms;

        void initUniformBuffers();

        virtual void initDescriptorSets();

        static vk::raii::DescriptorSetLayout *c_descriptor_set_layout;
        ViewProjectionTransform m_view_projection;
        LightInfo m_lights[MAX_LIGHTS];
        std::vector<vk::raii::Buffer> m_view_projection_buffers;
        std::vector<VmaAllocation> m_view_projection_buffer_allocations;
        std::vector<vk::raii::Buffer> m_light_list_buffers;
        std::vector<VmaAllocation> m_light_list_buffer_allocations;
    };

    class ModelUniformSet : public UniformSet {
    public:
        ModelUniformSet();
        ModelUniformSet(Uniforms *uniforms);
        ModelUniformSet(const ModelUniformSet &other) = delete;
        ModelUniformSet(ModelUniformSet &&other);

        virtual ~ModelUniformSet();

        ModelUniformSet &operator=(const ModelUniformSet &other) = delete;
        ModelUniformSet &operator=(ModelUniformSet &&other);

        static void initDescriptorSetLayout(System *gfx, Uniforms *owner);
        static const vk::raii::DescriptorSetLayout &descriptorSetLayout();

        void setTransform(const glm::mat4x4 &model);
        void updateModelBuffer(uint32_t buffer_index);

    protected:
        friend class Uniforms;

        void initUniformBuffers();

        virtual void initDescriptorSets();

        static vk::raii::DescriptorSetLayout *c_descriptor_set_layout;
        glm::mat4x4 m_model_transform;
        std::vector<vk::raii::Buffer> m_model_buffers;
        std::vector<VmaAllocation> m_model_buffer_allocations;
    };
}

#endif
