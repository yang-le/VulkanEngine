#version 450

layout(location = 0) in vec3 aNormalPosition;
layout(location = 1) in vec3 aVertexPosition;
layout(location = 2) in vec2 aTextureCoord;

layout(binding = 0) uniform m_proj_t {
    mat4 m_proj;
};
layout(binding = 1) uniform m_view_t {
    mat4 m_view;
};
layout(binding = 2) uniform m_model_t {
    mat4 m_model;
};

layout(location = 0) out vec4 vPosWorld;
layout(location = 1) out mat4 vWorldToScreen;

void main(void) {
    vec4 posWorld = m_model * vec4(aVertexPosition, 1.0);
    vPosWorld = posWorld.xyzw / posWorld.w;
    vWorldToScreen = m_proj * m_view;

    gl_Position = vWorldToScreen * posWorld;
}
