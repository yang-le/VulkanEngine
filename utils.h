#pragma once

#include <fstream>
#include <vector>

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>


struct VertexPC
{
  float x, y, z, w;   // Position
  float r, g, b, a;   // Color
};

struct VertexPT
{
  float x, y, z, w;   // Position data
  float u, v;         // texture u,v
};


static const VertexPC coloredCubeData[] =
{
  // red face
  { -1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
  { -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
  {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
  {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
  { -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
  {  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 0.0f, 1.0f },
  // green face
  { -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
  {  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
  { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
  { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
  {  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
  {  1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 0.0f, 1.0f },
  // blue face
  { -1.0f,  1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
  { -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
  { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
  { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
  { -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
  { -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f, 1.0f },
  // yellow face
  {  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
  {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
  {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
  {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
  {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
  {  1.0f, -1.0f, -1.0f, 1.0f,    1.0f, 1.0f, 0.0f, 1.0f },
  // magenta face
  {  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
  { -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
  {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
  {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
  { -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
  { -1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 0.0f, 1.0f, 1.0f },
  // cyan face
  {  1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
  {  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
  { -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
  { -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
  {  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
  { -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f, 1.0f, 1.0f },
};

static const VertexPT texturedCubeData[] =
{
  // left face
  { -1.0f, -1.0f, -1.0f, 1.0f,    1.0f, 0.0f },
  { -1.0f,  1.0f,  1.0f, 1.0f,    0.0f, 1.0f },
  { -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 0.0f },
  { -1.0f,  1.0f,  1.0f, 1.0f,    0.0f, 1.0f },
  { -1.0f, -1.0f, -1.0f, 1.0f,    1.0f, 0.0f },
  { -1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 1.0f },
  // front face
  { -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 0.0f },
  {  1.0f, -1.0f, -1.0f, 1.0f,    1.0f, 0.0f },
  {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 1.0f },
  { -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 0.0f },
  {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 1.0f },
  { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f },
  // top face
  { -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f },
  {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f },
  {  1.0f, -1.0f, -1.0f, 1.0f,    1.0f, 1.0f },
  { -1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 1.0f },
  { -1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 0.0f },
  {  1.0f, -1.0f, -1.0f, 1.0f,    1.0f, 0.0f },
  // bottom face
  { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 0.0f },
  {  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 1.0f },
  { -1.0f,  1.0f,  1.0f, 1.0f,    0.0f, 1.0f },
  { -1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 0.0f },
  {  1.0f,  1.0f, -1.0f, 1.0f,    1.0f, 0.0f },
  {  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 1.0f },
  // right face
  {  1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f },
  {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f },
  {  1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 1.0f },
  {  1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f },
  {  1.0f,  1.0f, -1.0f, 1.0f,    0.0f, 1.0f },
  {  1.0f, -1.0f, -1.0f, 1.0f,    0.0f, 0.0f },
  // back face
  { -1.0f,  1.0f,  1.0f, 1.0f,    1.0f, 1.0f },
  {  1.0f,  1.0f,  1.0f, 1.0f,    0.0f, 1.0f },
  { -1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f },
  { -1.0f, -1.0f,  1.0f, 1.0f,    1.0f, 0.0f },
  {  1.0f,  1.0f,  1.0f, 1.0f,    0.0f, 1.0f },
  {  1.0f, -1.0f,  1.0f, 1.0f,    0.0f, 0.0f },
};


// vertex shader with (P)osition and (C)olor in and (C)olor out
const std::string vertexShaderText_PC_C = R"(
#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 0) uniform buffer
{
  mat4 mvp;
} uniformBuffer;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main()
{
  outColor = inColor;
  gl_Position = uniformBuffer.mvp * pos;
}
)";

// vertex shader with (P)osition and (T)exCoord in and (T)exCoord out
const std::string vertexShaderText_PT_T = R"(
#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 0) uniform buffer
{
  mat4 mvp;
} uniformBuffer;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec2 outTexCoord;

void main()
{
  outTexCoord = inTexCoord;
  gl_Position = uniformBuffer.mvp * pos;
}
)";

// fragment shader with (C)olor in and (C)olor out
const std::string fragmentShaderText_C_C = R"(
#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 color;

layout (location = 0) out vec4 outColor;

void main()
{
  outColor = color;
}
)";

// fragment shader with (T)exCoord in and (C)olor out
const std::string fragmentShaderText_T_C = R"(
#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D tex;

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

void main()
{
  outColor = texture(tex, inTexCoord);
}
)";


inline glm::mat4x4 createModelViewProjectionClipMatrix(vk::Extent2D const & extent)
{
  float fov = glm::radians( 45.0f );
  if ( extent.width > extent.height )
  {
    fov *= static_cast<float>( extent.height ) / static_cast<float>( extent.width );
  }

  glm::mat4x4 model      = glm::mat4x4( 1.0f );
  glm::mat4x4 view       = glm::lookAt( glm::vec3( -5.0f, 3.0f, -10.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, -1.0f, 0.0f ) );
  glm::mat4x4 projection = glm::perspective( fov, 1.0f, 0.1f, 100.0f );
  // clang-format off
  glm::mat4x4 clip = glm::mat4x4( 1.0f,  0.0f, 0.0f, 0.0f,
                                  0.0f, -1.0f, 0.0f, 0.0f,
                                  0.0f,  0.0f, 0.5f, 0.0f,
                                  0.0f,  0.0f, 0.5f, 1.0f );  // vulkan clip space has inverted y and half z !
  // clang-format on
  return clip * projection * view * model;
}


inline std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open a file!");
    }

    auto fileSize = file.tellg();
    std::vector<char> fileBuffer(fileSize);

    file.seekg(std::ios::beg);
    file.read(fileBuffer.data(), fileSize);
    file.close();

    return fileBuffer;
}
