#version 450

#include "constants.glsl"

layout(location = 0) in vec3 vFragPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTextureCoord;

layout(binding = 3) uniform uKd_t {
    vec3 uKd;
};
layout(binding = 4) uniform uKs_t {
    vec3 uKs;
};
layout(binding = 5) uniform uLightPos_t {
    vec3 uLightPos;
};
layout(binding = 6) uniform uCameraPos_t {
    vec3 uCameraPos;
};
layout(binding = 7) uniform uLightIntensity_t {
    float uLightIntensity;
};
layout(binding = 8) uniform uTextureSample_t {
    int uTextureSample;
};
layout(binding = 9) uniform sampler2D uSampler;

layout(location = 0) out vec4 fragColor;

#include "blinnPhong.glsl"

void main(void) {
    vec3 color;
    if(uTextureSample == 1) {
        color = texture(uSampler, vTextureCoord).rgb;
    } else {
        color = uKd;
    }

    color = pow(color, gamma);
    color = blinnPhong(color, 0.05, color, uLightIntensity, uKs, uLightIntensity, uLightPos, uCameraPos, vFragPos, vNormal, 32);
    color = pow(color, inv_gamma);

    fragColor = vec4(color, 1.0);
}
