#include "gltf.h"

#include <iostream>

struct Mesh : gltf::Shader {
    Mesh(Engine& engine, const glm::mat4& model) : engine(engine), gltf::Shader("marry.gltf", "phong", engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};
        write_uniform(2, model);  // model matrix
    }

    virtual ~Mesh() override {
        // erase texture to avoid double free
        textures.erase(9);
    }

    virtual void init() override {
        Shader::init();

        write_uniform(3,
                      glm::vec3(model.model.materials[0].pbrMetallicRoughness.baseColorFactor[0],
                                model.model.materials[0].pbrMetallicRoughness.baseColorFactor[1],
                                model.model.materials[0].pbrMetallicRoughness.baseColorFactor[2]),
                      vk::ShaderStageFlagBits::eFragment);  // baseColorFactor
        write_uniform(4, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // specular
        write_uniform(5, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // light position
        write_uniform(6, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // camera position
        write_uniform(7, 5000.0f,
                      vk::ShaderStageFlagBits::eFragment);  // light intensity
        write_uniform(8, 1,
                      vk::ShaderStageFlagBits::eFragment);  // use texture

        textures[9] = model.textures[0];
    }

    virtual void update() override {
        Shader::update();

        auto time = engine.get_time();
        write_uniform(5, glm::vec3(sin(time * 1.5) * 100, cos(time) * 150, cos(time * 0.5) * 100));
        write_uniform(6, engine.get_player().position);
    }

    Engine& engine;
};

int main(int argc, char* argv[]) {
    try {
        Engine engine(1600, 900);

        auto player = std::make_unique<Player>(engine, glm::radians(75.0f), 1600.0 / 900.0, 0.1, 1000);
        player->position = {-20, 180, 250};

        engine.set_player(std::move(player));
        engine.add_mesh(std::make_unique<Mesh>(engine, glm::scale(glm::mat4(1), glm::vec3(52))));
        engine.vulkan.addRenderPass();

        engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what();
    } catch (...) {
        std::cerr << "Unknow exception!";
    }

    return 0;
}
