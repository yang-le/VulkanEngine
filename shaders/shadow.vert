#version 450

layout(location = 0) in vec3 aNormalPosition;
layout(location = 1) in vec3 aVertexPosition;
layout(location = 2) in vec2 aTextureCoord;

layout(binding = 0) uniform uLightMVP_t {
  mat4 uLightMVP;
};

void main(void) {
  gl_Position = uLightMVP * vec4(aVertexPosition, 1.0);
}
