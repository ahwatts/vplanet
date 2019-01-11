#version 450

const int MAX_LIGHTS = 10;
struct LightInfo {
    vec3 direction;
    bool enabled;
};

layout(location = 0) in float inHeight;
layout(location = 1) in vec3 inNormal;

layout(set = 0, binding = 1) uniform LightList {
    LightInfo lights[MAX_LIGHTS];
};

layout(location = 0) out vec4 outColor;

void main(void) {
    vec3 color = vec3(0.0, 0.0, 0.0);
    if (inHeight < 2.00) {
        color = vec3(0.8, 0.7, 0.4);
    } else if (inHeight < 2.08) {
        color = vec3(0.2, 0.6, 0.2);
    } else if (inHeight < 2.15) {
        color = vec3(0.5, 0.4, 0.3);
    } else {
        color = vec3(0.8, 0.8, 0.8);
    }

    // No specular highlight for the terrain. Just an ambient and a diffuse
    // term.
    int enabled_lights = 0;
    vec3 diffuse_colors[MAX_LIGHTS];
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled) {
            enabled_lights += 1;
            diffuse_colors[i] = color * dot(inNormal, -1 * lights[i].direction);
        }
    }

    vec3 ambient_color = color;

    vec3 diffuse_color = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled) {
            diffuse_color += (1.0 / enabled_lights) * diffuse_colors[i];
        }
    }
    diffuse_color = clamp(diffuse_color, 0.0, 1.0);

    outColor = vec4(0.1*ambient_color + 0.9*diffuse_color, 1.0);
}
