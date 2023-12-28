#version 450

#include "constants.h"

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput uShadowMap;

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
layout(binding = 9) uniform uTextureSample_t {
    int uTextureSample;
};
layout(binding = 10) uniform sampler2D uSampler;

layout(location = 0) out vec4 fragColor;

float unpack(vec4 rgbaDepth) {
    const vec4 bitShift = vec4(1.0, 1.0 / 256.0, 1.0 / (256.0 * 256.0), 1.0 / (256.0 * 256.0 * 256.0));
    return dot(rgbaDepth, bitShift);
}

float useShadowMap(subpassInput shadowMap, vec4 shadowCoord) {
    float depth = unpack(subpassLoad(shadowMap));
    return (shadowCoord.z > depth + EPS) ? exp(10.0 * (depth - shadowCoord.z)) : 1.0;
    // return (shadowCoord.z > depth + EPS) ? 0.0 : 1.0;
}

#include "blinnPhong.h"

void main(void) {
    float visibility;
    vec3 shadowCoord = vPositionFromLight.xyz / vPositionFromLight.w;
    shadowCoord = (shadowCoord + 1.0) / 2.0;  // convert position to uv
    visibility = useShadowMap(uShadowMap, vec4(shadowCoord, 1.0));

    vec3 color;
    if(uTextureSample == 1) {
        color = texture(uSampler, vTextureCoord).rgb;
    } else {
        color = uKd;
    }

    fragColor = vec4(blinnPhong(color) * visibility, 1.0);
}
