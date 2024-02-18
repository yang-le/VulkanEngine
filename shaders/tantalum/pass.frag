#version 450
#include "tantalum/preamble.glsl"

layout(binding = 0) uniform sampler2D Frame;

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(texture(Frame, vTexCoord).rgb, 1.0);
}
