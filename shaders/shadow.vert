#version 450

layout(location = 1) in vec3 aVertexPosition;

layout(binding = 0) uniform uLightVP_t {
  mat4 uLightVP;
};
layout(binding = 2) uniform m_model_t {
  mat4 m_model;
};

void main(void) {
  gl_Position = uLightVP * m_model * vec4(aVertexPosition, 1.0);
}
