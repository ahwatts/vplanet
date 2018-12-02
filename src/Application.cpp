// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "vulkan.h"
#include "Application.h"

Application::Application(GLFWwindow *window)
    : m_window{window},
      m_window_width{0},
      m_window_height{0},
      m_gfx{window},
      m_present{&m_gfx},
      m_terrain_renderer{&m_gfx}
{
    glfwGetFramebufferSize(window, &m_window_width, &m_window_height);
    glfwSetWindowUserPointer(m_window, this);
}

Application::~Application() {
    dispose();
}

void Application::init() {
    m_gfx.init(true);
    m_present.init(true);
    m_terrain_renderer.init();
}

void Application::dispose() {
    m_terrain_renderer.dispose();
    m_present.dispose();
    m_gfx.dispose();
}
