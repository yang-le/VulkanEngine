#pragma once

#include <string>
#include <map>
#include <fstream>

#include "vulkan.h"
#include "player.h"
#include "settings.h"


struct Engine;

struct Shader {
    Shader(Engine* engine);
    ~Shader();

    void update();
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

    Vulkan::Buffer vertex;
    std::vector<std::pair<vk::Format, uint32_t>> vert_format;
    std::map<std::string, Vulkan::Buffer> uniforms;
    vk::ShaderModule vert_shader = {};
    vk::ShaderModule frag_shader = {};

    Engine* engine;
    Vulkan* vulkan;
    Player* player;
};
