#include <iostream>

#include "Camera.h"

const glm::vec3 Camera::DEFAULT_FOCUS_POINT{0.0, 0.0, 0.0};
const glm::vec3 Camera::DEFAULT_POSITION{1.0, 0.0, 0.0};

Camera::Camera()
: m_focus_point{DEFAULT_FOCUS_POINT},
  m_position{DEFAULT_POSITION}
{}

Camera::Camera(const glm::vec3 &position, const glm::vec3 &focus)
: m_focus_point{focus},
  m_position{position}
{}

void Camera::reset() {
    m_focus_point = DEFAULT_FOCUS_POINT;
    m_position = DEFAULT_POSITION;
}

void Camera::reset(const glm::vec3 &position, const glm::vec3 &focus) {
    m_position = position;
    m_focus_point = focus;
}

void Camera::rotate(float dtheta, float dphi) {
    glm::vec3 y_axis{0.0, 1.0, 0.0};
    glm::vec3 cam_dir = m_position - m_focus_point;
    float radius = glm::length(cam_dir);
    cam_dir = glm::normalize(cam_dir);

    cam_dir = glm::rotate(cam_dir, dtheta, y_axis);

    glm::vec3 horizontal = glm::normalize(glm::cross(y_axis, cam_dir));
    cam_dir = glm::rotate(cam_dir, dphi, horizontal);

    float angle = glm::dot(cam_dir, y_axis);
    if (angle > 0.99863 || angle < -0.99863) {
        cam_dir = glm::rotate(cam_dir, -dphi, horizontal);
    }

    m_position = m_focus_point + cam_dir*radius;
}

void Camera::zoom(float dr) {
    glm::vec3 cam_dir = m_position - m_focus_point;
    float radius = glm::length(cam_dir);
    cam_dir = cam_dir / radius;

    radius += dr;
    if (radius < 1) {
        radius = 1;
    } else if (radius > 100) {
        radius = 100;
    }

    m_position = m_focus_point + cam_dir*radius;
}

glm::mat4x4 Camera::viewTransformation() const {
    return glm::lookAt(m_position, m_focus_point, glm::vec3{0.0, 1.0, 0.0});
}
