#pragma once

#include "player.h"
#include "settings.h"
#include "vulkan.h"

struct Engine;

struct Shader {
    Shader(Engine *engine);
    ~Shader();

    virtual void init();
    virtual void update();
    void load(const std::string &shader_name);

    template <typename T>
    void write(const std::string &name, const T &data) {
        auto &buffer = uniforms[name];
        if (!buffer.data) buffer = vulkan->createUniformBuffer(sizeof(T));

        memcpy(buffer.data, &data, sizeof(T));
    }

    template <typename T, size_t Size>
    void write(const T (&data)[Size]) {
        vertex = vulkan->createVertexBuffer(data);
    }

    template <size_t Size>
    void write(const std::string &name, const char (&filename)[Size]) {
        write(name, std::string(filename));
    }

    void write(const std::string &name, const std::string &filename);

    std::vector<Vulkan::Buffer> get_uniforms() {
        std::vector<Vulkan::Buffer> result;
        for (auto &it : uniforms) result.push_back(it.second);
        return result;
    }

    std::vector<Vulkan::Texture> get_textures() {
        std::vector<Vulkan::Texture> result;
        for (auto &it : textures) result.push_back(it.second);
    }

    Vulkan::Buffer vertex;
    std::vector<std::pair<vk::Format, uint32_t>> vert_format;
    std::map<std::string, Vulkan::Buffer> uniforms;
    std::map<std::string, Vulkan::Texture> textures;
    vk::ShaderModule vert_shader = {};
    vk::ShaderModule frag_shader = {};

    Engine *engine;
    Vulkan *vulkan;
    Player *player;
};
