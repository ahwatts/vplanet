// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <chrono>
#include <iostream>

#include "glm_defines.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "gfx/Uniforms.h"
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
    glfwSetKeyCallback(m_window, keypressCallback);
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

    Ocean ocean{1.97, 5};
    m_gfx.setOceanGeometry(ocean.vertices(), ocean.indices());

    gfx::ViewProjectionTransform vp_xform{};

    vp_xform.view = glm::lookAt(
        glm::vec3{0.0, 0.0, 5.0},
        glm::vec3{0.0, 0.0, 0.0},
        glm::vec3{0.0, 1.0, 0.0});
    vp_xform.projection = glm::perspectiveFov(
        20.0f,
        static_cast<float>(m_window_width),
        static_cast<float>(m_window_height),
        0.1f, 100.0f);
    vp_xform.projection[1][1] *= -1;
    m_gfx.setViewProjectionTransform(vp_xform);
    m_gfx.enableLight(0, { -1.0, -1.0, -1.0 });

    uint32_t num_images = m_gfx.swapchain().images().size();
    for (uint32_t i = 0; i < num_images; ++i) {
        m_gfx.writeViewProjectionTransform(i);
        m_gfx.writeLightList(i);
    }

    m_gfx.recordCommandBuffers();
}

void Application::dispose() {
    m_gfx.dispose();
}

void Application::run() {
    glm::mat4x4 model;

    try {
        static auto start_time = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(m_window)) {
            auto current_time = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
            model = glm::rotate(glm::mat4x4{1.0}, time * glm::radians(15.0f), glm::vec3{0.0, 1.0, 0.0});

            uint32_t image_index = m_gfx.startFrame();
            m_gfx.setTerrainTransform(model);
            m_gfx.setOceanTransform(model);
            m_gfx.writeTerrainTransform(image_index);
            m_gfx.writeOceanTransform(image_index);
            m_gfx.drawFrame(image_index);
            m_gfx.presentFrame(image_index);
            glfwPollEvents();
        }
        m_gfx.waitIdle();
    } catch (std::runtime_error &ex) {
        m_gfx.waitIdle();
        throw;
    }
}

void Application::keypressCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    Application *app = (Application*)glfwGetWindowUserPointer(window);
    if (app != nullptr) {
        app->handleKeypress(window, key, scancode, action, mods);
    }
}

void Application::handleKeypress(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    } else {
        const char *key_name = glfwGetKeyName(key, scancode);
        if (key_name == nullptr) {
            std::cout << "Unknown key";
        } else {
            std::cout << key_name << " key";
        }

        if (action == GLFW_PRESS) {
            std::cout << " press";
        } else if (action == GLFW_RELEASE) {
            std::cout << " release";
        } else if (action == GLFW_REPEAT) {
            std::cout << " repeat";
        } else {
            std::cout << " unknown action";
        }

        std::cout << " with";
        if (mods == 0) {
            std::cout << " no mods";
        } else {
            if (mods & GLFW_MOD_SHIFT) {
                std::cout << " shift";
            }
            if (mods & GLFW_MOD_CONTROL) {
                std::cout << " control";
            }
            if (mods & GLFW_MOD_ALT) {
                std::cout << " alt";
            }
            if (mods & GLFW_MOD_SUPER) {
                std::cout << " super";
            }
        }
        std::cout << std::endl;
    }
}
