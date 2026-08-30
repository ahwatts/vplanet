#ifndef _VPLANET_CAMERA_H_
#define _VPLANET_CAMERA_H_

#include "glm.h"

class Camera {
public:
    Camera();
    Camera(const glm::vec3 &position, const glm::vec3 &focus);

    glm::vec3 focusPoint() const { return m_focus_point; }
    void focusPoint(const glm::vec3 &new_focus) { m_focus_point = new_focus; }

    glm::vec3 position() const { return m_position; }
    void position(const glm::vec3 &new_position) { m_position = new_position; }

    void reset();
    void rotate(float dtheta, float dphi);
    void zoom(float dr);

    glm::mat4x4 viewTransformation() const;

    static const glm::vec3 DEFAULT_POSITION, DEFAULT_FOCUS_POINT;

private:
    glm::vec3 m_focus_point, m_position;
    // double m_radius, m_theta, m_phi;
};

#endif
