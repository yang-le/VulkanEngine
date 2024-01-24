#version 450

#include "constants.glsl"

layout(location = 0) in vec3 vFragPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTextureCoord;
layout(location = 3) in vec4 vPositionFromLight;
layout(location = 4) in float vDepth;

layout(binding = 4) uniform uKd_t {
    vec3 uKd;
};
layout(binding = 10) uniform sampler2D uSampler;
layout(binding = 11) uniform sampler2D uShadowMap;
layout(binding = 12) uniform sampler2D uSamplerNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outDepth;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out float outShadow;
layout(location = 4) out vec4 outPos;

void LocalBasis(vec3 n, out vec3 b1, out vec3 b2) {
    float sign_ = sign(n.z);
    if(n.z == 0.0) {
        sign_ = 1.0;
    }
    float a = -1.0 / (sign_ + n.z);
    float b = n.x * n.y * a;
    b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
    b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}

vec3 ApplyTangentNormalMap() {
    vec3 t, b;
    LocalBasis(vNormal, t, b);
    vec3 nt = texture(uSamplerNormal, vTextureCoord).xyz * 2.0 - 1.0;
    if(nt == vec3(-1))
        nt = vec3(0, 0, 1);
    nt = normalize(nt.x * t + nt.y * b + nt.z * vNormal);
    return nt;
}

#include "shadow.glsl"

void main(void) {
    // outShadow = useShadowMap(uShadowMap, vPositionFromLight / vPositionFromLight.w);
    // outShadow = PCF(uShadowMap, vPositionFromLight / vPositionFromLight.w, FILTER_SIZE);
    outShadow = PCSS(uShadowMap, vPositionFromLight / vPositionFromLight.w);

    outColor = texture(uSampler, vTextureCoord);
    if(outColor == vec4(0))
        outColor = vec4(uKd, 1.0);

    outDepth = vDepth;
    outNormal = vec4(ApplyTangentNormalMap(), 1.0);
    outPos = vec4(vFragPos, 1.0);
}
