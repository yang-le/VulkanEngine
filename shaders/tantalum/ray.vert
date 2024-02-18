#version 450
#include "tantalum/preamble.glsl"

layout(binding = 0) uniform sampler2D PosDataA;
layout(binding = 1) uniform sampler2D PosDataB;
layout(binding = 2) uniform sampler2D RgbData;
layout(binding = 3) uniform Aspect_t {
    float Aspect;
};

layout(location = 0) in vec3 TexCoord;

layout(location = 0) out vec3 vColor;

void main() {
    vec2 posA = texture(PosDataA, TexCoord.xy).xy;
    vec2 posB = texture(PosDataB, TexCoord.xy).xy;
    vec2 pos = mix(posA, posB, TexCoord.z);
    vec2 dir = posB - posA;
    float biasCorrection = clamp(length(dir) / max(abs(dir.x), abs(dir.y)), 1.0, 1.414214);

    gl_Position = vec4(pos.x / Aspect, pos.y, 0.0, 1.0);
    vColor = texture(RgbData, TexCoord.xy).rgb * biasCorrection;
}
