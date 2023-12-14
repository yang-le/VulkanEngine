#version 450

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 marker_color;
layout(location = 1) in vec2 uv;

layout(binding = 4) uniform sampler2D u_texture_0;


void main() {
    fragColor = texture(u_texture_0, uv);
    fragColor.rgb += marker_color;
}
