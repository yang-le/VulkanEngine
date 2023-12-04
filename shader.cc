#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "shader.h"
#include "engine.h"
#include <stdint.h>


namespace {
    std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open a file!");
        }

        auto fileSize = file.tellg();
        std::vector<char> fileBuffer(1 + fileSize);

        file.seekg(std::ios::beg);
        file.read(fileBuffer.data(), fileSize);
        file.close();

        return fileBuffer;
    }
}

Shader::Shader(Engine* engine)
    : engine(engine), vulkan(&engine->vulkan), player(&engine->player)
{
}

Shader::~Shader() {
    vulkan->destroyVertexBuffer(vertex);
    for (auto& uniform : uniforms)
        vulkan->destroyUniformBuffer(uniform.second);
    for (auto& texture : textures)
        vulkan->destroyTexture(texture.second);
    vulkan->destroyShaderModule(frag_shader);
    vulkan->destroyShaderModule(vert_shader);
}

void Shader::init() {
    auto proj = player->proj;
    proj[1][1] *= -1;
    write("m_proj", proj);
    write("m_view", player->view);
}

void Shader::update() {
    write("m_view", player->view);
}

void Shader::load(const std::string& shader_name)
{
    vert_shader = vulkan->createShaderModule(vk::ShaderStageFlagBits::eVertex, readFile("shaders/" + shader_name + ".vert").data());
    frag_shader = vulkan->createShaderModule(vk::ShaderStageFlagBits::eFragment, readFile("shaders/" + shader_name + ".frag").data());
}

void Shader::write(const std::string& name, const std::string& filename) {
    auto& texture = textures[name];
    if (!texture.sampler) {
        int width, height;
        std::string path = "assets/" + filename;
        stbi_uc* image = stbi_load(path.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
        if (!image)
            throw std::runtime_error("Failed to load a Texture file! (" + filename + ")");

        texture = vulkan->createTexture({(uint32_t)width, (uint32_t)height}, image);
    }
}
