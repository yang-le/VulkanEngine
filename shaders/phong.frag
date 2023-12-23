#version 450

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(location = 0) in vec3 vFragPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTextureCoord;

layout(push_constant) uniform drawId_t { uint drawId; };
layout(binding = 3) uniform uKd_t { vec3 uKd; };
layout(binding = 4) uniform uKs_t { vec3 uKs; };
layout(binding = 5) uniform uLightPos_t { vec3 uLightPos; };
layout(binding = 6) uniform uCameraPos_t { vec3 uCameraPos; };
layout(binding = 7) uniform uLightIntensity_t { float uLightIntensity; };
layout(binding = 8) uniform sampler2D uSampler;

layout(location = 0) out vec4 fragColor;

void main(void) {
    vec3 color;
    if (drawId == 1) {
        color = pow(texture(uSampler, vTextureCoord).rgb, gamma);
    } else {
        color = uKd;
    }

    vec3 ambient = 0.05 * color;

    vec3 lightDir = normalize(uLightPos - vFragPos);
    vec3 normal = normalize(vNormal);
    float diff = max(dot(lightDir, normal), 0.0);
    float light_atten_coff = uLightIntensity / length(uLightPos - vFragPos);
    vec3 diffuse = diff * light_atten_coff * color;

    vec3 viewDir = normalize(uCameraPos - vFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 35.0);
    vec3 specular = uKs * light_atten_coff * spec;

    fragColor = vec4(pow(ambient + diffuse + specular, inv_gamma), 1.0);
}
