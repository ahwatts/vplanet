struct ViewProjectionTransformation {
    float4x4 view;
    float4x4 projection;
}

static const int MAX_LIGHTS = 10;

struct LightInfo {
    float3 direction;
    bool enabled;
}

struct VertexInput {
    float3 position;
    float4 color;
    float3 normal;
}

struct VertexOutput {
    float4 position : SV_Position;
    float4 color;
    float3 normal;
    float3 eye_dir;
}

ConstantBuffer<ViewProjectionTransformation> xforms;
ConstantBuffer<float4x4> model_xform;

[shader("vertex")]
VertexOutput vs_main(VertexInput in) {
    VertexOutput out;

    // float4x4 view_inv = inverse(xforms.view);
    float4x4 view_inv = float4x4(
        float4(1.0, 0.0, 0.0, 0.0),
        float4(0.0, 1.0, 0.0, 0.0),
        float4(0.0, 0.0, 1.0, 0.0),
        float4(0.0, 0.0, 0.0, 1.0)
    );
    float4 wld_vert_pos4 = mul(model_xform, float4(in.position, 1.0));
    float3 wld_vert_pos = wld_vert_pos4.xyz / wld_vert_pos4.w;

    float4 wld_eye_pos4 = mul(view_inv, float4(0.0, 0.0, 0.0, 1.0));
    float3 wld_eye_pos = wld_eye_pos4.xyz / wld_eye_pos4.w;

    out.position = mul(xforms.projection, mul(xforms.view, mul(model_xform, float4(in.position, 1.0))));
    out.color = in.color;
    out.normal = normalize(mul(float3x3(model_xform), in.normal));
    out.eye_dir = normalize(wld_eye_pos - wld_vert_pos);

    return out;
}

ConstantBuffer<LightInfo[MAX_LIGHTS]> lights;

[shader("fragment")]
float4 fs_main(VertexOutput in) : SV_Target {
    int enabled_lights = 0;
    float3 diffuse_colors[MAX_LIGHTS];
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled) {
            enabled_lights += 1;
            diffuse_colors[i] = in.color.rgb * dot(in.normal, -1 * lights[i].direction);
        }
    }

    float3 ambient_color = in.color.rgb;

    float3 diffuse_color = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled) {
            diffuse_color += (1.0 / enabled_lights) * diffuse_colors[i];
        }
    }
    diffuse_color = clamp(diffuse_color, 0.0, 1.0);

    float specular_pow = 12.0;
    float3 specular_color = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled) {
            float3 reflected = normalize(reflect(lights[i].direction, in.normal));
            float specular_cos = dot(reflected, in.eye_dir);
            if (specular_cos > 0) {
                float specular = pow(specular_cos, specular_pow);
                specular_color += specular * float3(0.5, 0.5, 1.0);
            }
        }
    }
    specular_color = clamp(specular_color, 0.0, 1.0);

    return float4(0.1 * ambient_color + 0.9 * diffuse_color + 0.5 * specular_color, in.color.a);
}
