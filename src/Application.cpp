// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "vulkan.h"
#include "Application.h"

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
}

void Application::dispose() {
    m_gfx.dispose();
}
