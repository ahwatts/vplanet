// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "vulkan.h"

#include "Application.h"
#include "Curve.h"
#include "Noise.h"
#include "Terrain.h"

Application::Application(GLFWwindow *window)
    : m_window{window},
      m_window_width{0},
      m_window_height{0},
      m_gfx{window}
{
    glfwGetFramebufferSize(window, &m_window_width, &m_window_height);
    glfwSetWindowUserPointer(m_window, this);
}

Application::~Application() {
    dispose();
}

void Application::init() {
    m_gfx.init(true);

    const Perlin base_noise{2.0, 2.0, 2.0};
    const Octave octave_noise{base_noise, 4, 0.3};
    CubicSpline spline;
    spline
        .addControlPoint(-1.0, -1.0)
        .addControlPoint(-0.5, -0.5)
        .addControlPoint(0.0, -0.1)
        .addControlPoint(0.5, 0.8)
        .addControlPoint(0.75, 1.2)
        .addControlPoint(1.0, 1.2);
    const Curve curved_noise{octave_noise, spline};

    Terrain terrain{2.0, 5, curved_noise};
    m_gfx.setTerrainGeometry(terrain.vertices(), terrain.elements());
}

void Application::dispose() {
    m_gfx.dispose();
}
