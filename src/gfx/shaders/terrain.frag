#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main(void) {
    float radius = length(inPosition);
    vec3 color = vec3(0.0, 0.0, 0.0);

    if (radius < 2.00) {
        color = vec3(0.8, 0.7, 0.4);
    } else if (radius < 2.08) {
        color = vec3(0.2, 0.6, 0.2);
    } else if (radius < 2.15) {
        color = vec3(0.5, 0.4, 0.3);
    } else {
        color = vec3(0.8, 0.8, 0.8);
    }

    outColor = vec4(color.rgb * inNormal.z, 1.0);
}
