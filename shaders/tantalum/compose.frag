#version 450
#include "tantalum/preamble.glsl"

layout(binding = 0) uniform sampler2D Frame;
layout(binding = 1) uniform Exposure_t {
    float Exposure;
};

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(pow(texture(Frame, vTexCoord).rgb * Exposure, vec3(1.0 / 2.2)), 1.0);
}
