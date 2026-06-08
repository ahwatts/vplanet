struct ViewProjectionTransformation {
    float4x4 view;
    float4x4 view_inv;
    float4x4 projection;
}

static const int MAX_LIGHTS = 10;

struct LightInfo {
    float3 direction;
    bool enabled;
}

struct VertexInput {
    float3 position;
    float3 normal;
}

struct VertexOutput {
    float4 position : SV_Position;
    float height;
    float3 normal;
}

ConstantBuffer<ViewProjectionTransformation> vp_xforms;
ConstantBuffer<float4x4> model_xform;

[shader("vertex")]
VertexOutput vs_main(VertexInput in) {
    VertexOutput out;
    out.position = mul(vp_xforms.projection, mul(vp_xforms.view, mul(model_xform, float4(in.position, 1.0))));
    out.height = length(in.position);
    out.normal = mul(float3x3(model_xform), in.normal);
    return out;
}

ConstantBuffer<LightInfo[MAX_LIGHTS]> lights;

[shader("fragment")]
float4 fs_main(VertexOutput in) : SV_Target {
    float3 color = float3(0.0, 0.0, 0.0);
    if (in.height < 2.00) {
        color = float3(0.8, 0.7, 0.4);
    } else if (in.height < 2.08) {
        color = float3(0.2, 0.6, 0.2);
    } else if (in.height < 2.15) {
        color = float3(0.5, 0.4, 0.3);
    } else {
        color = float3(0.8, 0.8, 0.8);
    }

    // No specular highlight for the terrain. Just an ambient and a diffuse
    // term.
    int enabled_lights = 0;
    float3 diffuse_colors[MAX_LIGHTS];
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled) {
            enabled_lights += 1;
            diffuse_colors[i] = color * dot(in.normal, -1 * lights[i].direction);
        }
    }

    float3 ambient_color = color;

    float3 diffuse_color = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled) {
            diffuse_color += (1.0 / enabled_lights) * diffuse_colors[i];
        }
    }
    diffuse_color = clamp(diffuse_color, 0.0, 1.0);

    return float4(0.1 * ambient_color + 0.9 * diffuse_color, 1.0);
}
