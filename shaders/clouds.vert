#version 450

layout(location = 0) in vec3 in_position;

layout(binding = 0) uniform m_proj_t { mat4 m_proj; };
layout(binding = 1) uniform m_view_t { mat4 m_view; };
layout(binding = 2) uniform u_time_t { float u_time; };

void main() {
    vec3 pos = in_position;
    pos.xz += 300 * sin(0.01 * u_time);
    gl_Position = m_proj * m_view * vec4(pos, 1.0);
}
