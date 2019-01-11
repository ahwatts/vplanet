#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(set = 0, binding = 0) uniform ViewProjectionTransformation {
    mat4x4 view;
    mat4x4 projection;
};

layout(set = 1, binding = 0) uniform ModelTransformation {
    mat4x4 model;
};

layout(location = 0) out float outHeight;
layout(location = 1) out vec3 outNormal;

void main(void) {
    gl_Position = projection * view * model * vec4(inPosition, 1.0);
    outHeight = length(inPosition);
    outNormal = normalize(mat3x3(model) * inNormal);
}
