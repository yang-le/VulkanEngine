#version 450

layout(location = 0) in vec3 vFragPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTextureCoord;

layout(binding = 3) uniform uKd_t {
    vec3 uKd;
};
layout(binding = 4) uniform uLightDir_t {
    vec3 uLightDir;
};
layout(binding = 5) uniform uCameraPos_t {
    vec3 uCameraPos;
};
layout(binding = 6) uniform uLightRadiance_t {
    vec3 uLightRadiance;
};
layout(binding = 7) uniform uLightPos_t {
    vec3 uLightPos;
};
layout(binding = 8) uniform uMetallic_t {
    float uMetallic;
};
layout(binding = 9) uniform uRoughness_t {
    float uRoughness;
};

layout(binding = 10) uniform sampler2D uAlbedoMap;
layout(binding = 11) uniform sampler2D uBRDFLut;
layout(binding = 12) uniform sampler2D uEavgLut;

layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float k = roughness * roughness / 2.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(vec3 F0, vec3 V, vec3 H) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - max(dot(H, V), 0.0), 0.0, 1.0), 5.0);
}

//https://blog.selfshadow.com/publications/s2017-shading-course/imageworks/s2017_pbs_imageworks_slides_v2.pdf
vec3 AverageFresnel(vec3 r, vec3 g) {
    return vec3(0.087237) + 0.0230685 * g - 0.0864902 * g * g + 0.0774594 * g * g * g + 0.782654 * r - 0.136432 * r * r + 0.278708 * r * r * r + 0.19744 * g * r + 0.0360605 * g * g * r - 0.2586 * g * r * r;
}

vec3 MultiScatterBRDF(float NdotL, float NdotV) {
    vec4 color = texture(uAlbedoMap, vTextureCoord);
    if(color == vec4(0))
        color = vec4(uKd, 0.0);

    vec3 albedo = pow(color.rgb, vec3(2.2));

    vec3 E_o = texture(uBRDFLut, vec2(NdotL, uRoughness)).xyz;
    vec3 E_i = texture(uBRDFLut, vec2(NdotV, uRoughness)).xyz;

    vec3 E_avg = texture(uEavgLut, vec2(0, uRoughness)).xyz;
    // copper
    vec3 edgetint = vec3(0.827, 0.792, 0.678);
    vec3 F_avg = AverageFresnel(albedo, edgetint);

    // calculate fms and missing energy here
    vec3 F_ms = (1.0 - E_o) * (1.0 - E_i) / (PI * (1.0 - E_avg));
    vec3 F_add = F_avg * E_avg / (1.0 - F_avg * (1.0 - E_avg));

    return F_add * F_ms;
}

void main(void) {
    vec4 color = texture(uAlbedoMap, vTextureCoord);
    if(color == vec4(0))
        color = vec4(uKd, 0.0);

    vec3 albedo = pow(color.rgb, vec3(2.2));

    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vFragPos);
    float NdotV = max(dot(N, V), 0.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, uMetallic);

    vec3 Lo = vec3(0.0);

    // calculate per-light radiance
    vec3 L = normalize(uLightDir);
    vec3 H = normalize(V + L);
    float distance = length(uLightPos - vFragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = uLightRadiance;

    float NDF = DistributionGGX(N, H, uRoughness);
    float G = GeometrySmith(N, V, L, uRoughness);
    vec3 F = fresnelSchlick(F0, V, H);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 Fmicro = numerator / max(denominator, 0.001);

    float NdotL = max(dot(N, L), 0.0);

    vec3 Fms = MultiScatterBRDF(NdotL, NdotV);
    vec3 BRDF = Fmicro + Fms;

    Lo += BRDF * radiance * NdotL;

    Lo = Lo / (Lo + vec3(1.0));
    Lo = pow(Lo, vec3(1.0 / 2.2));
    fragColor = vec4(Lo, 1.0);
}
