#pragma once

#include "player.h"
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

template <typename Ret, size_t I, typename... Tuples, size_t Size, size_t... J>
constexpr Ret hstack_helper(std::index_sequence<J...>, const std::array<Tuples, Size>... arrays) {
    return {std::get<J>(std::tuple_cat(arrays[I]...))...};
}

template <typename Ret, size_t... I, typename... Tuples, size_t Size>
constexpr std::array<Ret, Size> hstack(std::index_sequence<I...>, const std::array<Tuples, Size>... arrays) {
    constexpr size_t TupleSize = std::tuple_size_v<decltype(std::tuple_cat(arrays[0]...))>;
    return {hstack_helper<Ret, I>(std::make_index_sequence<TupleSize>{}, arrays...)...};
}

template <typename Ret, typename... Tuples, size_t Size>
constexpr std::array<Ret, Size> hstack(const std::array<Tuples, Size>... arrays) {
    return hstack<Ret>(std::make_index_sequence<Size>{}, arrays...);
}

struct IShader {
    virtual ~IShader() = default;

    virtual void init() = 0;
    virtual void update() = 0;
    virtual void draw() = 0;

    virtual void load() = 0;
    virtual void attach(uint32_t subpass = 0) = 0;

    virtual void pre_attach(){};
    // virtual void post_draw(){};
};

struct Shader : IShader {
    Shader() = default;
    Shader(const std::string &name, Engine &engine);
    virtual ~Shader();

    virtual void init();
    virtual void update() { write_uniform(1, camera->view); }
    virtual void draw() {
        if (draw_id != -1) vulkan->draw(draw_id, vertex);
    }

    virtual void load();
    virtual void attach(uint32_t subpass = 0);

    template <typename T>
    void write_uniform(int binding, const T &data, vk::ShaderStageFlags stage = {}) {
        auto &buffer = uniforms[binding];
        if (!buffer.data) buffer = vulkan->createUniformBuffer(sizeof(T));

        memcpy(buffer.data, &data, sizeof(T));
        if (stage) buffer.stage = stage;
    }

    template <typename T, size_t Size>
    void write_vertex(const std::array<T, Size> &data) {
        if (vertex.data) vulkan->destroyVertexBuffer(vertex);
        vertex = vulkan->createVertexBuffer(data.data(), sizeof(T), Size);
    }

    template <typename T>
    void write_vertex(const std::vector<T> &data) {
        if (vertex.data) vulkan->destroyVertexBuffer(vertex);
        vertex = vulkan->createVertexBuffer(data.data(), sizeof(T), data.size());
    }

    void write_texture(int binding, const std::string &filename, uint32_t layers = 1);
    void write_texture(int binding, const void *data, uint32_t width, uint32_t height, uint32_t layers = 1);
    void write_texture(int binding, std::initializer_list<std::string> filenames,
                       std::initializer_list<uint32_t> index);

    Vulkan::Buffer vertex;
    std::vector<vk::Format> vert_formats;
    std::map<int, Vulkan::Buffer> uniforms;
    std::map<int, Vulkan::Texture> textures;
    vk::ShaderModule vert_shader = {};
    vk::ShaderModule frag_shader = {};
    std::string shader_name;
    vk::CullModeFlags cull_mode = vk::CullModeFlagBits::eBack;
    uint32_t draw_id = -1;

    Vulkan *vulkan;
    const Camera *camera;
};

template <size_t N = 0>
struct MultiShader : IShader {
    using Container =
        std::conditional_t<N == 0, std::vector<std::unique_ptr<IShader>>, std::array<std::unique_ptr<IShader>, N>>;

    virtual void attach(uint32_t subpass = 0) override {
        for (auto &shader : shaders) shader->attach(subpass);
    }

    virtual void pre_attach() override {
        for (auto &shader : shaders) shader->pre_attach();
    }
    virtual void init() override {
        for (auto &shader : shaders) shader->init();
    }
    virtual void update() override {
        for (auto &shader : shaders) shader->update();
    }
    virtual void load() override {
        for (auto &shader : shaders) shader->load();
    }
    virtual void draw() override {
        for (auto &shader : shaders) shader->draw();
    }

    Container shaders;
};

template <size_t N>
struct MultiSubpassShader : MultiShader<N> {
    MultiSubpassShader(Vulkan &vulkan) : vulkan(vulkan) {}

    virtual void attach(uint32_t) override {
        // this is the only way to attach the subpass shaders
        for (unsigned i = 0; i < N; ++i) this->shaders[i]->attach(i);
    }

    virtual void draw() override {
        unsigned i;
        for (i = 0; i < N - 1; ++i) {
            this->shaders[i]->draw();
            vulkan.nextSubpass();
        }
        this->shaders[i]->draw();
    }

    Vulkan &vulkan;
};

template <size_t N>
struct MultiPassShader : MultiShader<N> {
    MultiPassShader(Vulkan &vulkan) : vulkan(vulkan) {}

    virtual void attach(uint32_t) override {
        for (auto &shader : this->shaders) {
            shader->pre_attach();
            shader->attach();
        }
    }

    virtual void draw() override {
        unsigned i;
        for (i = 0; i < N - 1; ++i) {
            this->shaders[i]->draw();
            vulkan.nextPass();
        }
        this->shaders[i]->draw();
    }

    Vulkan &vulkan;
};
