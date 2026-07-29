// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <iostream>
#include <string>
#include <tuple>
#include <utility>

#include "vulkan.h"

#include "Application.h"

// These are "logical sizes," after display scale factors.
const int WIDTH = 1024;
const int HEIGHT = 768;

void initGLFW(int width, int height, const char *title, GLFWwindow **window);
void handleGLFWError(int code, const char *desc);
void bailout(const std::string &msg);
std::pair<int, int> logicalSizeToGlfwScreenCoordinates(int width, int height);

int main(int argc, char **argv) {
    GLFWwindow *window;
    initGLFW(WIDTH, HEIGHT, "Planet Demo", &window);    

    try {
        Application app{window};
        app.run();
    } catch (std::runtime_error &ex) {
        std::cerr << "Error running vplanet: " << ex.what() << "\n";
    }

    glfwTerminate();
    return 0;
}

void initGLFW(int width, int height, const char *title, GLFWwindow **window) {
    glfwSetErrorCallback(handleGLFWError);
    if (!glfwInit()) {
        bailout("Could not initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    std::tie(width, height) = logicalSizeToGlfwScreenCoordinates(width, height);
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

// The WIDTH and HEIGHT values above are intended as "screen coordinate"
// distances, rather than pixels. However, when creating windows, GLFW appears
// to use them as pixel sizes on Windows and X11 (or rather, Windows and X11
// treat them as pixel sizes), and MacOS treats them as scaled device units.
// This function handles that difference.
std::pair<int, int> logicalSizeToGlfwScreenCoordinates(int width, int height) {
#ifdef __APPLE__
    return std::make_pair(width, height);
#else
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    if (monitor == nullptr) {
        bailout("No monitor available");
    }

    float xscale = 0.0f, yscale = 0.0f;
    glfwGetMonitorContentScale(monitor, &xscale, &yscale);
    return std::make_pair(static_cast<int>(xscale * width), static_cast<int>(yscale * height));
#endif
}