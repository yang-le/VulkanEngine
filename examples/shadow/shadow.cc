#include <iostream>

#include "gltf.h"

namespace {
auto lightView = glm::lookAt(glm::vec3(0, 80, 80), glm::vec3(0), glm::vec3(0, 1, 0));
auto lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1e-2f, 1000.0f);
}  // namespace

struct PhongShadow : gltf::Shader {
    PhongShadow(Engine& engine, const std::string& gltf_file, const std::string& shader_file, const glm::mat4& model)
        : engine(engine), gltf::Shader(gltf_file, shader_file, engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};

        write_uniform(2, model);                        // model matrix
        write_uniform(3, lightProjection * lightView);  // light VP
    }

    virtual ~PhongShadow() override {
        // erase texture to avoid double free
        if (!model.textures.empty()) textures.erase(10);
        textures.erase(11);
    }

    virtual void init() override {
        Shader::init();

        write_uniform(4,
                      glm::vec3(model.model.materials[0].pbrMetallicRoughness.baseColorFactor[0],
                                model.model.materials[0].pbrMetallicRoughness.baseColorFactor[1],
                                model.model.materials[0].pbrMetallicRoughness.baseColorFactor[2]),
                      vk::ShaderStageFlagBits::eFragment);  // baseColorFactor
        write_uniform(5, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // specular
        write_uniform(6, glm::vec3(0, 80, 80),
                      vk::ShaderStageFlagBits::eFragment);  // light position
        write_uniform(7, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // camera position
        write_uniform(8, 5000.0f,
                      vk::ShaderStageFlagBits::eFragment);  // light intensity

        if (!model.textures.empty()) textures[10] = model.textures[0];
    }

    virtual void pre_attach() override { textures[11] = engine.get_offscreen_depth_texture(); }

    virtual void update() override {
        Shader::update();

        write_uniform(7, engine.get_player().position);
    }

    Engine& engine;
};

struct Shadow : gltf::Shader {
    Shadow(Engine& engine, const std::string& gltf_file, const glm::mat4& model)
        : gltf::Shader(gltf_file, "shadow", engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};

        write_uniform(0, lightProjection * lightView);  // light VP
        write_uniform(2, model);
    }

    virtual void init() override {
        // disable base class init
    }

    virtual void update() override {
        // disable base class update
    }
};

template <size_t N>
struct Shadows : MultiShader<N> {
    Shadows(Engine& engine) : engine(engine) {}
    virtual void pre_attach() override {
        auto renderPassBuilder = engine.vulkan.makeRenderPassBuilder(vk::Format::eD16Unorm, false, false, true);
        engine.vulkan.addRenderPass(renderPassBuilder);
    }

    Engine& engine;
};

template <size_t N>
struct PhongShadows : MultiShader<N> {
    PhongShadows(Engine& engine) : engine(engine) {}

    virtual void pre_attach() override {
        MultiShader<N>::pre_attach();
        engine.vulkan.addRenderPass();
    }

    Engine& engine;
};

int main(int argc, char* argv[]) {
    try {
        int width = 1600, height = 900;
        int xpos = -1, ypos = -1;
        for (;;) {
            Engine engine(width, height, 2);
            if (xpos != -1 && ypos != -1) engine.set_window_pos(xpos, ypos);

            auto player = std::make_unique<Player>(engine, glm::radians(75.0f), 1600.0 / 900.0, 1e-2, 1000);
            player->position = {30, 30, 30};
            engine.set_player(std::move(player));

            auto floorModel = glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, 0, -30)), glm::vec3(4));
            auto marryModel1 = glm::scale(glm::mat4(1), glm::vec3(20));
            auto marryModel2 = glm::scale(glm::translate(glm::mat4(1), glm::vec3(40, 0, -40)), glm::vec3(10));

            auto firstPass = std::make_unique<Shadows<3>>(engine);
            firstPass->shaders[0] = std::make_unique<Shadow>(engine, "floor.gltf", floorModel);
            firstPass->shaders[1] = std::make_unique<Shadow>(engine, "marry.gltf", marryModel1);
            firstPass->shaders[2] = std::make_unique<Shadow>(engine, "marry.gltf", marryModel2);

            auto secondPass = std::make_unique<PhongShadows<3>>(engine);
            secondPass->shaders[0] = std::make_unique<PhongShadow>(engine, "floor.gltf", "floor", floorModel);
            secondPass->shaders[1] = std::make_unique<PhongShadow>(engine, "marry.gltf", "marry", marryModel1);
            secondPass->shaders[2] = std::make_unique<PhongShadow>(engine, "marry.gltf", "marry", marryModel2);

            auto mesh = std::make_unique<MultiPassShader<2>>(engine.vulkan);
            mesh->shaders[0] = std::move(firstPass);
            mesh->shaders[1] = std::move(secondPass);
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
