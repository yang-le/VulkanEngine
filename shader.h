#pragma once

#include "player.h"
#include "settings.h"
#include "vulkan.h"

template <typename... T, size_t verSize, size_t indSize, size_t... I>
constexpr std::array<std::tuple<T...>, indSize> get_data(const std::array<std::tuple<T...>, verSize> &vertices,
                                                         const std::array<size_t, indSize> &indices,
                                                         std::index_sequence<I...>) {
    return {vertices[indices[I]]...};
}

template <typename... T, size_t verSize, size_t indSize>
constexpr decltype(auto) get_data(const std::array<std::tuple<T...>, verSize> &vertices,
                                  const std::array<size_t, indSize> &indices) {
    return get_data(vertices, indices, std::make_index_sequence<indSize>{});
}

template <size_t I, typename... Tuples, size_t Size>
constexpr decltype(auto) hstack_helper(const std::array<Tuples, Size>... arrays) {
    return std::tuple_cat(arrays[I]...);
}

template <typename... Tuples, size_t Size, size_t... I>
constexpr std::array<decltype(std::tuple_cat(Tuples{}...)), Size> hstack_helper(
    std::index_sequence<I...>, const std::array<Tuples, Size>... arrays) {
    return {hstack_helper<I>(arrays...)...};
}

template <typename... Tuples, size_t Size>
constexpr decltype(auto) hstack(const std::array<Tuples, Size>... arrays) {
    return hstack_helper(std::make_index_sequence<Size>{}, arrays...);
}

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
    void write_vertex(const std::array<T, Size> &data) {
        vertex = vulkan->createVertexBuffer(data.data(), sizeof(T), Size);
    }

    template <typename T>
    void write_vertex(const std::vector<T> &data) {
        vertex = vulkan->createVertexBuffer(data.data(), sizeof(T), data.size());
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
