#pragma once

#include "vulkan.h"
#include "player.h"
#include "settings.h"


struct Engine;

struct Shader {
    Shader(Engine* engine);
    ~Shader();

    virtual void init();
    virtual void update();
    void load(const std::string& shader_name);

    template<typename T>
    void write(const std::string& name, const T& data) {
        auto& buffer = uniforms[name];
        if (!buffer.data)
            buffer = vulkan->createUniformBuffer(sizeof(T));

        memcpy(buffer.data, &data, sizeof(T));
    }

    template<typename T, size_t Size>
    void write(const T (&data)[Size]) {
        vertex = vulkan->createVertexBuffer(data);
    }

    template<size_t Size>
    void write(const std::string& name, const char (&filename)[Size]) {
        write(name, std::string(filename));
    }

    void write(const std::string& name, const std::string& filename);

    Vulkan::Buffer vertex;
    std::vector<std::pair<vk::Format, uint32_t>> vert_format;
    std::map<std::string, Vulkan::Buffer> uniforms;
    std::map<std::string, Vulkan::Texture> textures;
    vk::ShaderModule vert_shader = {};
    vk::ShaderModule frag_shader = {};

    Engine* engine;
    Vulkan* vulkan;
    Player* player;
};
