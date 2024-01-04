#version 450

#include "constants.glsl"

layout(location = 0) in vec3 vFragPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTextureCoord;
layout(location = 3) in vec4 vPositionFromLight;

layout(binding = 4) uniform uKd_t {
    vec3 uKd;
};
layout(binding = 5) uniform uKs_t {
    vec3 uKs;
};
layout(binding = 6) uniform uLightPos_t {
    vec3 uLightPos;
};
layout(binding = 7) uniform uCameraPos_t {
    vec3 uCameraPos;
};
layout(binding = 8) uniform uLightIntensity_t {
    float uLightIntensity;
};
layout(binding = 10) uniform sampler2D uSampler;
layout(binding = 11) uniform sampler2D uShadowMap;

layout(location = 0) out vec4 fragColor;

float useShadowMap(vec4 shadowCoord) {
    float depth = texture(uShadowMap, shadowCoord.st).r;
    return (shadowCoord.z > depth + EPS) ? 0.1 : 1.0;
}

#include "blinnPhong.glsl"

void main(void) {
    float visibility = useShadowMap(vPositionFromLight / vPositionFromLight.w);
    vec3 color = texture(uSampler, vTextureCoord).rgb;

    color = pow(color, gamma);
    color = blinnPhong(color, 0.05, color, uLightIntensity, uKs, uLightIntensity, uLightPos, uCameraPos, vFragPos, vNormal, 32);
    color = pow(color, inv_gamma);

    fragColor = vec4(color * visibility, 1.0);
}
