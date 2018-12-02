// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_GFX_TERRAIN_RENDERER_H_
#define _VPLANET_GFX_TERRAIN_RENDERER_H_

#include "../vulkan.h"

namespace gfx {
    class System;

    class TerrainRenderer {
    public:
        TerrainRenderer(System *system);
        ~TerrainRenderer();

        void init();
        void dispose();

    private:
        System *m_system;
    };
}

#endif
