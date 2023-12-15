#include "shader.h"

#include <fstream>
#include <iostream>

#include "engine.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {
std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to load file: " + filename);
    }

    auto fileSize = file.tellg();
    std::vector<char> fileBuffer(1 + fileSize);

    file.seekg(std::ios::beg);
    file.read(fileBuffer.data(), fileSize);
    file.close();

    return fileBuffer;
}
}  // namespace

Shader::Shader(const std::string& name, Engine* engine)
    : shader_name(name), engine(engine), vulkan(&engine->vulkan), player(&engine->player) {}

Shader::~Shader() {
    vulkan->destroyVertexBuffer(vertex);
    for (auto& uniform : uniforms) vulkan->destroyUniformBuffer(uniform.second);
    for (auto& texture : textures) vulkan->destroyTexture(texture.second);
}

void Shader::init() {
    auto proj = player->proj;
    proj[1][1] *= -1;
    write_uniform(0, proj);
    write_uniform(1, player->view);
}

void Shader::update() { write_uniform(1, player->view); }

void Shader::load() {
    try {
        vert_shader = vulkan->createShaderModule(vk::ShaderStageFlagBits::eVertex,
                                                 readFile("shaders/" + shader_name + ".vert").data());
    } catch (const std::exception&) {
        std::cerr << "When compiling file: shaders/" + shader_name + ".vert\n";
        throw;
    }
    try {
        frag_shader = vulkan->createShaderModule(vk::ShaderStageFlagBits::eFragment,
                                                 readFile("shaders/" + shader_name + ".frag").data());
    } catch (const std::exception&) {
        std::cerr << "When compiling file: shaders/" + shader_name + ".frag\n";
        throw;
    }
}

void Shader::attach() {
    draw_id = vulkan->attachShader(vert_shader, frag_shader, vertex, vert_formats, uniforms, textures, cull_mode);
    vulkan->destroyShaderModule(frag_shader);
    vulkan->destroyShaderModule(vert_shader);
}

void Shader::draw() { vulkan->draw(draw_id); }

void Shader::write_texture(int binding, const std::string& filename, uint32_t layers) {
    auto& texture = textures[binding];
    if (!texture.sampler) {
        int width, height;
        std::string path = "assets/" + filename;
        stbi_uc* image = stbi_load(path.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
        if (!image) throw std::runtime_error("Failed to load texture file: assets/" + filename);

        texture = vulkan->createTexture({(uint32_t)width, (uint32_t)height / layers}, image, layers);
        stbi_image_free(image);
    }
}
