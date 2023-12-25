#include <iostream>

#include "gltf.h"

struct Phong : gltf::Shader {
    Phong(Engine& engine, const std::string& gltf_file, const glm::mat4& model, bool use_texture = true)
        : engine(engine), gltf::Shader(gltf_file, "phong_shadow", engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};
        write_uniform(2, model);  // model matrix
        write_uniform(8, use_texture ? 1 : 0, vk::ShaderStageFlagBits::eFragment);
    }

    virtual ~Phong() override {
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
        write_uniform(5, glm::vec3(0, 80, 80),
                      vk::ShaderStageFlagBits::eFragment);  // light position
        write_uniform(6, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // camera position
        write_uniform(7, 5000.0f,
                      vk::ShaderStageFlagBits::eFragment);  // light intensity

        textures[9] = model.textures.size() ? model.textures[0] : Vulkan::Texture{};
    }

    virtual void update() override {
        Shader::update();

        write_uniform(6, engine.get_player().position);
    }

    Engine& engine;
};

int main(int argc, char* argv[]) {
    // we have 3 attachments in total:
    // attachment 0: the framebuffer color
    // attachment 1: the depth GBuffer, which format passed into the renderPassBuilder
    // attachment 2: the framebuffer depth
    auto renderPassBuilder =
        Vulkan::makeRenderPassBuilder(vk::Format::eR32G32B32A32Sfloat)
            .addSubpass({1})       // subpass 0, writes to depth attachment
            .addSubpass({0}, {1})  // subpass 1, reads from depth attachment and writes to framebuffer
            .dependOn(0);          // subpass 1 depends on subpass 0

    try {
        Engine engine(1600, 900);
        engine.init(renderPassBuilder);

        auto player = std::make_unique<Player>(engine, glm::radians(75.0f), 1600.0 / 900.0, 1e-2, 1000);
        player->position = {30, 30, 30};

        engine.set_player(std::move(player));
        engine.add_mesh(std::make_unique<Phong>(
            engine, "floor.gltf", glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, 0, -30)), glm::vec3(4)), false));
        engine.add_mesh(std::make_unique<Phong>(engine, "Marry.gltf", glm::scale(glm::mat4(1), glm::vec3(20))));
        engine.add_mesh(std::make_unique<Phong>(
            engine, "Marry.gltf", glm::scale(glm::translate(glm::mat4(1), glm::vec3(40, 0, -40)), glm::vec3(10))));

        engine.loop();
        engine.destroy();
    } catch (const std::exception& e) {
        std::cerr << e.what();
    } catch (...) {
        std::cerr << "Unknow exception!";
    }

    return 0;
}
