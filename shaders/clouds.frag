#version 450

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(location = 0) out vec4 fragColor;

void main() {
    float fog_dist = gl_FragCoord.z / gl_FragCoord.w;
    vec3 col = mix(cloud_color, bg_color, 1.0 - exp(-0.000001 * fog_dist * fog_dist));

    fragColor = vec4(col, 0.8);
}
