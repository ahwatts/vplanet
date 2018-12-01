// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include <string>

#include "vulkan.h"

#include "Application.h"

const int WIDTH = 1024;
const int HEIGHT = 768;

void initGLFW(int width, int height, const char *title, GLFWwindow **window);
void handleGLFWError(int code, const char *desc);
void bailout(const std::string &msg);

int main(int argc, char **argv) {
    GLFWwindow *window;
    initGLFW(WIDTH, HEIGHT, "Planet Demo", &window);
    Application app{window};

    try {
        app.init();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    } catch (std::runtime_error &ex) {
        std::cerr << "Error running vplanet: " << ex.what() << "\n";
    }

    app.dispose();
    glfwTerminate();
    return 0;
}

void initGLFW(int width, int height, const char *title, GLFWwindow **window) {
    glfwSetErrorCallback(handleGLFWError);
    if (!glfwInit()) {
        bailout("Could not initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    *window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!*window) {
        bailout("Could not create window");
    }
}

void handleGLFWError(int code, const char *desc) {
    std::cerr << "GLFW Error Code " << code << "\n"
              << desc << "\n";
}

void bailout(const std::string &msg) {
    std::cerr << msg << "\n";
    glfwTerminate();
    std::exit(1);
}
