#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;

layout(set = 0, binding = 0) uniform ViewProjectionTransformation {
    mat4x4 view;
    mat4x4 projection;
};

layout(set = 1, binding = 0) uniform ModelTransformation {
    mat4x4 model;
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outEyeDir;

void main(void) {
    mat4x4 view_inv = inverse(view);
    vec4 wld_vert_pos4 = model * vec4(inPosition, 1.0);
    vec3 wld_vert_pos = wld_vert_pos4.xyz / wld_vert_pos4.w;

    vec4 wld_eye_pos4 = view_inv * vec4(0.0, 0.0, 0.0, 1.0);
    vec3 wld_eye_pos = wld_eye_pos4.xyz / wld_eye_pos4.w;

    gl_Position = projection * view * model * vec4(inPosition, 1.0);
    outColor = inColor;
    outNormal = normalize(mat3x3(model) * inNormal);
    outEyeDir = normalize(wld_eye_pos - wld_vert_pos);
}
