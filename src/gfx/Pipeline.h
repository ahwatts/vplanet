// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_PIPELINE_H_
#define _VPLANET_GFX_PIPELINE_H_

#include "../vulkan.h"

#include "Resource.h"

namespace gfx {
    class Renderer;

    class Pipeline {
    public:
        Pipeline();
        Pipeline(Renderer *renderer);
        Pipeline(const Pipeline &other) = delete;
        Pipeline(Pipeline &&other) = default;

        virtual ~Pipeline();

        Pipeline &operator=(const Pipeline &other) = delete;
        Pipeline &operator=(Pipeline &&other) = default;

    protected:
        virtual void initPipeline() = 0;

        Renderer *m_renderer;
        vk::raii::Pipeline m_pipeline;
    };
}

#endif
