// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <chrono>

#include "vulkan.h"

#include "glm_defines.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Application.h"
#include "Curve.h"
#include "Noise.h"
#include "Terrain.h"

Application::Application(GLFWwindow *window)
    : m_window{window},
      m_window_width{0},
      m_window_height{0},
      m_gfx{window},
      m_xforms{}
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

    m_xforms.model = glm::mat4x4{1.0};
    m_xforms.view = glm::lookAt(
        glm::vec3{0.0, 0.0, 5.0},
        glm::vec3{0.0, 0.0, 0.0},
        glm::vec3{0.0, 1.0, 0.0});
    m_xforms.projection = glm::perspectiveFov(
        20.0f,
        static_cast<float>(m_window_width),
        static_cast<float>(m_window_height),
        0.1f, 100.0f);

    uint32_t num_images = m_gfx.swapchain().images().size();
    for (uint32_t i = 0; i < num_images; ++i) {
        m_gfx.setTransforms(m_xforms, i);
    }

    m_gfx.recordCommandBuffers();
}

void Application::dispose() {
    m_gfx.dispose();
}

void Application::run() {
    static auto start_time = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(m_window)) {
        auto current_time = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
        m_xforms.model = glm::rotate(glm::mat4x4{1.0}, time * glm::radians(15.0f), glm::vec3{0.0, 1.0, 0.0});

        uint32_t image_index = m_gfx.startFrame();
        m_gfx.setTransforms(m_xforms, image_index);
        m_gfx.drawFrame(image_index);
        m_gfx.presentFrame(image_index);
        glfwPollEvents();
    }
    m_gfx.waitIdle();
}
