#version 450

layout(location = 0) in vec3 aNormalPosition;
layout(location = 1) in vec3 aVertexPosition;
layout(location = 2) in vec2 aTextureCoord;

layout(binding = 0) uniform m_proj_t { mat4 m_proj; };
layout(binding = 1) uniform m_view_t { mat4 m_view; };
layout(binding = 2) uniform m_model_t { mat4 m_model; };
layout(binding = 3) uniform uLightMVP_t { mat4 uLightMVP; };

layout(location = 0) out vec3 vFragPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vTextureCoord;
layout(location = 3) out vec4 vPositionFromLight;

void main(void) {
  vec4 FragPos = m_model * vec4(aVertexPosition, 1.0);

  vFragPos = FragPos.xyz;
  vNormal = (m_model * vec4(aNormalPosition, 1.0)).xyz;
  vTextureCoord = aTextureCoord;
  vPositionFromLight = uLightMVP * vec4(aVertexPosition, 1.0);

  gl_Position = m_proj * m_view * FragPos;
}
