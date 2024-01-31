#include <iostream>

#include "gltf.h"

namespace {
auto lightPos = glm::vec3(100, 0, 100);
auto lightDir = glm::vec3(0, 0, -1);
auto lightRadiance = glm::vec3(1);

auto lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(1, 0, 0));
auto lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1e-2f, 1000.0f);

auto transform = glm::scale(glm::rotate(glm::mat4(1), glm::pi<float>(), glm::vec3(0, 1, 0)), glm::vec3(2));
}  // namespace

struct Mesh : gltf::PrimitiveShader {
    Mesh(Engine& engine, gltf::Primitive& primitive, const std::string& name, const glm::mat4& modelMat,
         const glm::vec3& offset, float roughness)
        : engine(engine), gltf::PrimitiveShader(primitive, name, engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};

        write_uniform(2, glm::translate(glm::mat4(1), offset) * transform * modelMat);  // model matrix
        write_uniform(9, roughness,
                      vk::ShaderStageFlagBits::eFragment);  // roughness
    }

    virtual void init() override {
        Shader::init();

        write_uniform(3, glm::vec3(0.7216, 0.451, 0.2), vk::ShaderStageFlagBits::eFragment);  // uKd
        write_uniform(4, -lightDir, vk::ShaderStageFlagBits::eFragment);                      // light dir
        write_uniform(5, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // camera position
        write_uniform(6, lightRadiance,
                      vk::ShaderStageFlagBits::eFragment);  // light intensity
        write_uniform(7, lightPos,
                      vk::ShaderStageFlagBits::eFragment);  // light position
        write_uniform(8, 1.0f,
                      vk::ShaderStageFlagBits::eFragment);  // metallic

        textures[10] = Vulkan::Texture();       // AlbedoMap
        write_texture(11, "GGX_E_LUT.png");     // BRDFLut
        write_texture(12, "GGX_Eavg_LUT.png");  // EavgLut
    }

    virtual void update() override {
        Shader::update();

        write_uniform(5, engine.get_player().position);
    }

    Engine& engine;
};

int main(int argc, char* argv[]) {
    try {
        Engine engine(1600, 900);

        auto player = std::make_unique<Player>(engine, glm::radians(75.0f), 1600.0 / 900.0, 1e-2, 1000);
        player->position = {0, 80, 300};

        engine.set_player(std::move(player));

        gltf::Model model(&engine.vulkan, "assets/ball.gltf");
        model.load();

        for (unsigned j = 0; j < 5; ++j)
            for (unsigned i = 0; i < 4; ++i) {
                engine.add_mesh(std::make_unique<Mesh>(engine, model.meshes[i].primitives[0], "KullaConty",
                                                       model.nodes[0].children[i].getMatrix(),
                                                       glm::vec3(180 - j * 80.0f, 60, 0), 0.15 + j * 0.2));
                engine.add_mesh(std::make_unique<Mesh>(engine, model.meshes[i].primitives[0], "pbr",
                                                       model.nodes[0].children[i].getMatrix(),
                                                       glm::vec3(180 - j * 80.0f, -60, 0), 0.15 + j * 0.2));
            }

        engine.vulkan.addRenderPass();

        engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what();
    } catch (...) {
        std::cerr << "Unknow exception!";
    }

    return 0;
}
