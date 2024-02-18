#include "shader.h"

#include <fstream>
#include <iostream>

#include "engine.h"
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

[[nodiscard("Don't forget to call stbi_image_free")]] std::tuple<stbi_uc*, uint32_t, uint32_t> loadImage(
    const std::string& filename) {
    int width, height;
    stbi_uc* image = stbi_load(filename.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
    if (!image) throw std::runtime_error("Failed to load texture file: " + filename);
    return {image, width, height};
}
}  // namespace

Shader::Shader(const std::string& name, Engine& engine)
    : vert_name(name), frag_name(name), vulkan(&engine.vulkan), camera(&engine.get_player()) {}

Shader::Shader(const std::string& vert_name, const std::string& frag_name, Engine& engine)
    : vert_name(vert_name), frag_name(frag_name), vulkan(&engine.vulkan), camera(&engine.get_player()) {}

Shader::~Shader() {
    vulkan->destroyVertexBuffer(vertex);
    for (auto& uniform : uniforms) vulkan->destroyUniformBuffer(uniform.second);
    for (auto& texture : textures) vulkan->destroyTexture(texture.second);
}

void Shader::init() {
    auto proj = camera->proj;
    proj[1][1] *= -1;
    write_uniform(0, proj);
    write_uniform(1, camera->view);
}

void Shader::load() {
    try {
        vert_shader = vulkan->createShaderModule(vk::ShaderStageFlagBits::eVertex,
                                                 readFile("shaders/" + vert_name + ".vert").data());
    } catch (const std::exception&) {
        std::cerr << "When compiling file: shaders/" + vert_name + ".vert\n";
        throw;
    }
    try {
        frag_shader = vulkan->createShaderModule(vk::ShaderStageFlagBits::eFragment,
                                                 readFile("shaders/" + frag_name + ".frag").data());
    } catch (const std::exception&) {
        std::cerr << "When compiling file: shaders/" + frag_name + ".frag\n";
        throw;
    }
}

void Shader::attach(uint32_t subpass) {
    draw_id = vulkan->attachShader(vert_shader, frag_shader, vertex.stride, vert_formats, uniforms, textures, subpass,
                                   cull_mode);
}

void Shader::write_texture(int binding, const std::string& filename, uint32_t layers) {
    void* image;
    uint32_t width, height;
    std::tie(image, width, height) = loadImage("assets/" + filename);
    write_texture(binding, image, width, height / layers, layers);
    stbi_image_free(image);
}

void Shader::write_texture(int binding, const void* data, uint32_t width, uint32_t height, uint32_t layers,
                           bool cubemap) {
    auto& texture = textures[binding];
    if (!texture.sampler) texture = vulkan->createTexture({(uint32_t)width, (uint32_t)height}, data, layers, cubemap);
}

void Shader::write_texture(int binding, std::initializer_list<std::string> filenames,
                           std::initializer_list<uint32_t> index) {
    uint32_t width, height;
    std::vector<stbi_uc*> images(filenames.size());

    // load all images
    auto filename = filenames.begin();
    std::tie(images[0], width, height) = loadImage("assets/" + *filename++);
    for (int i = 1; i < filenames.size(); ++i) {
        uint32_t width_, height_;
        std::tie(images[i], width_, height_) = loadImage("assets/" + *filename++);
        assert(width == width_);
        assert(height == height_);
    }

    // stack data
    uint32_t layers = (uint32_t)index.size() / 3 + 1;
    std::vector<stbi_uc> data(3 * 4 * width * height * layers);

    // layer 0 is always empty, start from layer 1
    auto it = index.begin();
    for (unsigned i = 1; i < layers; ++i, it += 3)
        for (unsigned j = 0; j < height; ++j)
            for (unsigned k = 0; k < 3; ++k)
                memcpy(&data[((i * height + j) * 3 + k) * 4 * width], &images[*(it + k)][j * 4 * width], 4 * width);

    // free all images
    for (int i = 0; i < filenames.size(); ++i) stbi_image_free(images[i]);

    write_texture(binding, data.data(), 3 * width, height, layers);
}

void Shader::write_texture(int binding, std::array<std::string, 6> filenames) {
    uint32_t width, height;
    std::array<stbi_uc*, 6> images;

    // load all images
    std::tie(images[0], width, height) = loadImage("assets/" + filenames[0]);
    assert(width == height);

    for (int i = 1; i < 6; ++i) {
        uint32_t width_, height_;
        std::tie(images[i], width_, height_) = loadImage("assets/" + filenames[i]);
        assert(width == width_);
        assert(height == height_);
    }

    std::vector<stbi_uc> data(4 * width * height * 6);
    for (int i = 0; i < 6; ++i) memcpy(&data[4 * width * height * i], images[i], 4 * width * height);

    // free all images
    for (int i = 0; i < 6; ++i) stbi_image_free(images[i]);

    write_texture(binding, data.data(), width, height, 6, true);
}
