#version 450

const int MAX_LIGHTS = 10;
struct LightInfo {
    vec3 direction;
    bool enabled;
};

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inEyeDir;

layout(set = 0, binding = 1) uniform LightList {
    LightInfo lights[MAX_LIGHTS];
};

layout(location = 0) out vec4 outColor;

void main(void) {
    int enabled_lights = 0;
    vec3 diffuse_colors[MAX_LIGHTS];
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled) {
            enabled_lights += 1;
            diffuse_colors[i] = inColor.rgb * dot(inNormal, -1 * lights[i].direction);
        }
    }

    vec3 ambient_color = inColor.rgb;

    vec3 diffuse_color = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled) {
            diffuse_color += (1.0 / enabled_lights) * diffuse_colors[i];
        }
    }
    diffuse_color = clamp(diffuse_color, 0.0, 1.0);

    float specular_pow = 12.0;
    vec3 specular_color = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled) {
            vec3 reflected = normalize(reflect(lights[i].direction, inNormal));
            float specular_cos = dot(reflected, inEyeDir);
            if (specular_cos > 0) {
                float specular = pow(specular_cos, specular_pow);
                specular_color += specular * vec3(0.5, 0.5, 1.0);
            }
        }
    }
    specular_color = clamp(specular_color, 0.0, 1.0);

    outColor = vec4(0.1*ambient_color + 0.9*diffuse_color + 0.5*specular_color, inColor.a);
}
