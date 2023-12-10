#pragma once

#include "player.h"
#include "settings.h"
#include "vulkan.h"

struct Engine;

struct Shader {
    Shader() = default;
    Shader(const std::string &name, Engine *engine);
    ~Shader();

    virtual void init();
    virtual void update();
    void load();

    template <typename T>
    void write_uniform(int binding, const T &data, vk::ShaderStageFlags stage = {}) {
        auto &buffer = uniforms[binding];
        if (!buffer.data) buffer = vulkan->createUniformBuffer(sizeof(T));

        memcpy(buffer.data, &data, sizeof(T));
        if (stage) buffer.stage = stage;
    }

    template <typename T, size_t Size>
    void write_vertex(const T (&data)[Size]) {
        vertex = vulkan->createVertexBuffer(data);
    }

    template <typename T>
    void write_vertex(const std::vector<T> &data) {
        vertex = vulkan->createVertexBuffer(data);
    }

    void write_texture(int binding, const std::string &filename, uint32_t layers = 1);

    Vulkan::Buffer vertex;
    std::vector<vk::Format> vert_formats;
    std::map<int, Vulkan::Buffer> uniforms;
    std::map<int, Vulkan::Texture> textures;
    vk::ShaderModule vert_shader = {};
    vk::ShaderModule frag_shader = {};
    std::string shader_name;
    vk::CullModeFlags cull_mode = vk::CullModeFlagBits::eBack;

    Engine *engine;
    Vulkan *vulkan;
    Player *player;
};
