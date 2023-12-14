#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(binding = 0) uniform m_proj_t { uniform mat4 m_proj; };
layout(binding = 1) uniform m_view_t { uniform mat4 m_view; };
layout(binding = 2) uniform m_model_t { uniform mat4 m_model; };
layout(binding = 3) uniform mode_id_t { uniform uint mode_id; };

const vec3 marker_colors[2] = vec3[2](vec3(1, 0, 0), vec3(0, 0, 1));

layout(location = 0) out vec3 marker_color;
layout(location = 1) out vec2 uv;


void main() {
    uv = in_tex_coord;
    marker_color = marker_colors[mode_id];
    gl_Position = m_proj * m_view * m_model * vec4((in_position - 0.5) * 1.01 + 0.5, 1.0);
}
