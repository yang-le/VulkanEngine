#version 450

layout(location = 0) in vec2 aVertexPosition;

layout(binding = 0) uniform m_proj_t {
    mat4 m_proj;
};
layout(binding = 1) uniform m_view_t {
    mat4 m_view;
};

layout(location = 0) out vec2 vScreenUV;
layout(location = 1) out mat4 vWorldToScreen;

void main(void) {
    vScreenUV = aVertexPosition * 0.5 + 0.5;
    vWorldToScreen = m_proj * m_view;

    gl_Position = vec4(aVertexPosition, 0.0, 1.0);
}
