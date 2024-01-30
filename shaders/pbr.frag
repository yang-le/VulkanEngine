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

layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
   // TODO: To calculate GGX NDF here

    return 1.0;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    // TODO: To calculate Smith G1 here

    return 1.0;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    // TODO: To calculate Smith G here

    return 1.0;
}

vec3 fresnelSchlick(vec3 F0, vec3 V, vec3 H) {
    // TODO: To calculate Schlick F here
    return vec3(1.0);
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

    vec3 L = normalize(uLightDir);
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);

    vec3 radiance = uLightRadiance;

    float NDF = DistributionGGX(N, H, uRoughness);
    float G = GeometrySmith(N, V, L, uRoughness);
    vec3 F = fresnelSchlick(F0, V, H);

    vec3 numerator = NDF * G * F;
    float denominator = max((4.0 * NdotL * NdotV), 0.001);
    vec3 BRDF = numerator / denominator;

    Lo += BRDF * radiance * NdotL;

    Lo = Lo / (Lo + vec3(1.0));
    Lo = pow(Lo, vec3(1.0 / 2.2));
    fragColor = vec4(Lo, 1.0);
}
