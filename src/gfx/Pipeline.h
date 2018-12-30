// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_PIPELINE_H_
#define _VPLANET_GFX_PIPELINE_H_

#include "../vulkan.h"

#include "Resource.h"

namespace gfx {
    class Renderer;

    class Pipeline {
    public:
        Pipeline(Renderer *renderer);
        virtual ~Pipeline();

        void init();
        void dispose();

    protected:
        virtual void initPipelineLayout() = 0;
        void cleanupPipelineLayout();

        virtual void initPipeline() = 0;
        void cleanupPipeline();

        Renderer *m_renderer;
        VkPipelineLayout m_pipeline_layout;
        VkPipeline m_pipeline;
    };
}

#endif
