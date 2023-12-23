#include "gltf.h"

#include <iostream>

struct Mesh : gltf::Shader {
    Mesh(Engine& engine) : engine(engine), gltf::Shader("Marry.gltf", "phong", engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};
    }

    virtual ~Mesh() override {
        // erase texture to avoid double free
        textures.erase(8);
    }

    virtual void init() override {
        Shader::init();
        write_uniform(2, glm::mat4(1));  // model matrix

        write_uniform(3, glm::vec3(0.270588, 0.552941, 0.87451),
                      vk::ShaderStageFlagBits::eFragment);  // baseColorFactor
        write_uniform(4, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // specular
        write_uniform(5, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // light position
        write_uniform(6, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // camera position
        write_uniform(7, 250.0f,
                      vk::ShaderStageFlagBits::eFragment);  // light intensity

        textures[8] = model.textures[0];
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
        engine.init();

        engine.add_mesh(std::make_unique<Mesh>(engine));

        engine.loop();
        engine.destroy();
    } catch (const std::exception& e) {
        std::cerr << e.what();
    } catch (...) {
        std::cerr << "Unknow exception!";
    }

    return 0;
}
