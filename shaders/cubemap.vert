#version 450

layout(location = 0) in vec3 inPos;

layout(binding = 0) uniform m_proj_t {
    mat4 m_proj;
};
layout(binding = 1) uniform m_view_t {
    mat4 m_view;
};

layout(location = 0) out vec3 outUVW;

void main() {
    outUVW = inPos;
    // Convert cubemap coordinates into Vulkan coordinate space
    outUVW.x *= -1.0;
    gl_Position = m_proj * m_view * vec4(inPos.xyz, 1.0);
}
