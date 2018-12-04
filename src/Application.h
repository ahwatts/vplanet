// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#ifndef _VPLANET_APPLICATION_H_
#define _VPLANET_APPLICATION_H_

#include "vulkan.h"
#include "gfx/System.h"
#include "gfx/TerrainRenderer.h"

class Application {
public:
    Application(GLFWwindow *window);
    ~Application();

    void init();
    void dispose();

private:
    GLFWwindow *m_window;
    int m_window_width, m_window_height;
    gfx::System m_gfx;
    gfx::TerrainRenderer m_terrain_renderer;
};

#endif
