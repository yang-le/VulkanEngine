#version 450

layout(location = 0) in vec3 aNormalPosition;
layout(location = 1) in vec3 aVertexPosition;
layout(location = 2) in vec2 aTextureCoord;
layout(location = 3) in vec3 aPrecomputeLT0;
layout(location = 4) in vec3 aPrecomputeLT1;
layout(location = 5) in vec3 aPrecomputeLT2;

layout(binding = 0) uniform m_proj_t {
    mat4 m_proj;
};
layout(binding = 1) uniform m_view_t {
    mat4 m_view;
};
layout(binding = 2) uniform m_model_t {
    mat4 m_model;
};
layout(binding = 3) uniform uPrecomputeL_t {
    mat3 uPrecomputeL_R;
    mat3 uPrecomputeL_G;
    mat3 uPrecomputeL_B;
};

layout(location = 0) out vec3 vColor;

void main(void) {
    vColor.r = dot(uPrecomputeL_R[0], aPrecomputeLT0) + dot(uPrecomputeL_R[1], aPrecomputeLT1) + dot(uPrecomputeL_R[2], aPrecomputeLT2);
    vColor.g = dot(uPrecomputeL_G[0], aPrecomputeLT0) + dot(uPrecomputeL_G[1], aPrecomputeLT1) + dot(uPrecomputeL_G[2], aPrecomputeLT2);
    vColor.b = dot(uPrecomputeL_B[0], aPrecomputeLT0) + dot(uPrecomputeL_B[1], aPrecomputeLT1) + dot(uPrecomputeL_B[2], aPrecomputeLT2);

    gl_Position = m_proj * m_view * m_model * vec4(aVertexPosition, 1.0);
}
