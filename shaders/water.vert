#version 450

#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec2 uv;

layout(binding = 0) uniform m_proj_t { mat4 m_proj; };
layout(binding = 1) uniform m_view_t { mat4 m_view; };


void main() {
    vec3 pos = in_position;
    pos.xz *= water_area;
    pos.xz -= 0.33 * water_area;

    pos.y += water_line;
    uv = in_tex_coord * water_area;
    gl_Position = m_proj * m_view * vec4(pos, 1.0);
}
