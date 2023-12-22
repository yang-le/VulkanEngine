#include <iostream>

#include "gltf.h"

struct Mesh : gltf::Shader {
    Mesh(Engine& engine) : gltf::Shader("Marry.gltf", "phong", engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};
    }

    virtual ~Mesh() override {
        // erase texture to avoid double free
        textures.erase(9);
    }

    virtual void init() override {
        Shader::init();

        write_uniform(2, glm::mat4(1));
        write_uniform(3, glm::vec3(0));
        write_uniform(4, glm::vec3(0));
        write_uniform(5, glm::vec3(0));
        write_uniform(6, glm::vec3(0));
        write_uniform(7, 0.0f);
        write_uniform(8, 1);

        textures[9] = model.textures[0];
    }
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
