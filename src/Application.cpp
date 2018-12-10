// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "vulkan.h"
#include "Application.h"

Application::Application(GLFWwindow *window)
    : m_window{window},
      m_window_width{0},
      m_window_height{0},
      m_gfx{window},
      m_terrain_renderer{&m_gfx},
      m_render_driver{&m_gfx}
{
    glfwGetFramebufferSize(window, &m_window_width, &m_window_height);
    glfwSetWindowUserPointer(m_window, this);
}

Application::~Application() {
    dispose();
}

void Application::init() {
    m_gfx.init(true);
    m_terrain_renderer.init();
    m_render_driver.init();
}

void Application::dispose() {
    m_render_driver.dispose();
    m_terrain_renderer.dispose();
    m_gfx.dispose();
}
