#include <iostream>

#include "gltf.h"

namespace {
auto lightPos = glm::vec3(-0.45, 5.40507, 0.637043);
auto lightDir = glm::vec3(0.39048811, -0.89896828, 0.19843153);
auto lightRadiance = glm::vec3(20);

auto lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(1, 0, 0));
auto lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1e-2f, 100.0f);
}  // namespace

struct SSR : Shader {
    SSR(Engine& engine) : engine(engine), Shader("ssr", engine) { vert_formats = {vk::Format::eR32G32Sfloat}; }

    virtual ~SSR() override {
        // erase texture to avoid double free
        textures.erase(6);
        textures.erase(7);
        textures.erase(8);
        textures.erase(9);
        textures.erase(10);
    }

    virtual void init() override {
        Shader::init();

        // Array for triangle that fills screen
        std::array<glm::vec2, 3> positions = {glm::vec2{3.0, -1.0}, {-1.0, -1.0}, {-1.0, 3.0}};
        write_vertex(positions);

        write_uniform(3, -lightDir, vk::ShaderStageFlagBits::eFragment);      // light dir
        write_uniform(4, glm::vec3(0), vk::ShaderStageFlagBits::eFragment);   // camera pos
        write_uniform(5, lightRadiance, vk::ShaderStageFlagBits::eFragment);  // light radiance
    }

    virtual void pre_attach() override {
        textures[6] = engine.get_offscreen_color_texture(0);
        textures[7] = engine.get_offscreen_color_texture(1);
        textures[8] = engine.get_offscreen_color_texture(2);
        textures[9] = engine.get_offscreen_color_texture(3);
        textures[10] = engine.get_offscreen_color_texture(4);
        engine.vulkan.addRenderPass();
    }

    virtual void update() override {
        Shader::update();

        write_uniform(4, engine.get_player().position);
    }

    Engine& engine;
};

struct Shadow : gltf::PrimitiveShader {
    Shadow(Engine& engine, gltf::Primitive& primitive, const glm::mat4& modelMat)
        : gltf::PrimitiveShader(primitive, "shadow", engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};

        write_uniform(0, lightProjection * lightView);  // light VP
        write_uniform(2, modelMat);                     // model matrix
    }

    virtual void init() override {
        // disable base class init
    }

    virtual void update() override {
        // disable base class update
    }
};

struct GBuffer : gltf::PrimitiveShader {
    GBuffer(Engine& engine, gltf::Primitive& primitive, const glm::mat4& modelMat)
        : engine(engine), gltf::PrimitiveShader(primitive, "gbuffer", engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};

        write_uniform(2, modelMat);                     // model matrix
        write_uniform(3, lightProjection * lightView);  // light VP
    }

    virtual ~GBuffer() override {
        // erase texture to avoid double free
        textures.erase(10);
        textures.erase(12);
        textures.erase(11);
    }

    virtual void init() override {
        Shader::init();

        write_uniform(4, primitive.baseColorFactor,
                      vk::ShaderStageFlagBits::eFragment);  // baseColorFactor

        if (primitive.baseColorTexture)
            textures[10] = *primitive.baseColorTexture;
        else
            textures[10] = {};

        if (primitive.normalTexture)
            textures[12] = *primitive.normalTexture;
        else
            textures[12] = {};
    }

    virtual void pre_attach() override { textures[11] = engine.get_offscreen_depth_texture(); }

    Engine& engine;
};

struct Shadows : MultiShader<12> {
    Shadows(Engine& engine) : engine(engine) {}
    virtual void pre_attach() override {
        auto renderPassBuilder = engine.vulkan.makeRenderPassBuilder(vk::Format::eD16Unorm, false, false, true);
        engine.vulkan.addRenderPass(renderPassBuilder);
    }

    Engine& engine;
};

struct GBuffers : MultiShader<12> {
    GBuffers(Engine& engine) : engine(engine) {}
    virtual void pre_attach() override {
        MultiShader<12>::pre_attach();
        auto renderPassBuilder = engine.vulkan
                                     .makeRenderPassBuilder({engine.get_surface_format(), vk::Format::eR32Sfloat,
                                                             vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32Sfloat,
                                                             vk::Format::eR16G16B16A16Sfloat, vk::Format::eD16Unorm},
                                                            false, true)
                                     .addSubpass({0, 1, 2, 3, 4});
        engine.vulkan.addRenderPass(renderPassBuilder);
    }

    Engine& engine;
};

int main(int argc, char* argv[]) {
    try {
        int width = 1600, height = 900;
        int xpos = -1, ypos = -1;

        for (;;) {
            Engine engine(width, height, 3);
            if (xpos != -1 && ypos != -1) engine.set_window_pos(xpos, ypos);

            auto player = std::make_unique<Player>(engine, glm::radians(75.0f), 1600.0 / 900.0, 1e-3, 1000);
            player->position = {4.18927, 1.0313, 2.07331};
            engine.set_player(std::move(player));

            gltf::Model model(&engine.vulkan, "assets/cave/cave.gltf");
            model.load();

            auto shadowPass = std::make_unique<Shadows>(engine);
            for (unsigned i = 0; i < shadowPass->shaders.size(); ++i)
                shadowPass->shaders[i] =
                    std::make_unique<Shadow>(engine, model.meshes[i].primitives[0], model.nodes[i + 2].getMatrix());

            auto gbufferPass = std::make_unique<GBuffers>(engine);
            for (unsigned i = 0; i < gbufferPass->shaders.size(); ++i)
                gbufferPass->shaders[i] =
                    std::make_unique<GBuffer>(engine, model.meshes[i].primitives[0], model.nodes[i + 2].getMatrix());

            auto mesh = std::make_unique<MultiPassShader<3>>(engine.vulkan);
            mesh->shaders[0] = std::move(shadowPass);
            mesh->shaders[1] = std::move(gbufferPass);
            mesh->shaders[2] = std::make_unique<SSR>(engine);
            engine.add_mesh(std::move(mesh));

            engine.quit_on_resize = true;
            engine.run();

            if (engine.quit_reason != Engine::WINDOW_RESIZE) break;

            width = engine.width;
            height = engine.height;
            xpos = engine.xpos;
            ypos = engine.ypos;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what();
    } catch (...) {
        std::cerr << "Unknow exception!";
    }

    return 0;
}
