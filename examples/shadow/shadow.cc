#include <iostream>

#include "gltf.h"

struct PhongShadow : gltf::Shader {
    PhongShadow(Engine& engine, const std::string& gltf_file, const std::string& shader_file, const glm::mat4& model,
                bool use_texture = true)
        : engine(engine), gltf::Shader(gltf_file, shader_file, engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};

        auto lightView = glm::lookAt(glm::vec3(0, 80, 80), glm::vec3(0), glm::vec3(0, 1, 0));
        auto lightProjection = glm::ortho(-800.0f, 800.0f, -450.0f, 450.0f, 1e-2f, 1000.0f);

        write_uniform(2, model);                                // model matrix
        write_uniform(3, lightProjection * lightView * model);  // light MVP
        write_uniform(9, use_texture ? 1 : 0, vk::ShaderStageFlagBits::eFragment);
    }

    virtual ~PhongShadow() override {
        // erase texture to avoid double free
        if (!model.textures.empty()) textures.erase(10);
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

        auto lightView = glm::lookAt(glm::vec3(0, 80, 80), glm::vec3(0), glm::vec3(0, 1, 0));
        auto lightProjection = glm::ortho(-800.0f, 800.0f, -450.0f, 450.0f, 1e-2f, 1000.0f);

        write_uniform(0, lightProjection * lightView * model);  // light MVP
    }

    virtual void init() override {
        // disable base class init
    }

    virtual void update() override {
        // disable base class update
    }
};

// TODO: It's not suitable for this shadow example to use a 2-subpass render
// Will back to try this using https://github.com/SaschaWillems/Vulkan/tree/master/examples/shadowmapping

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
        Engine engine(1600, 900, renderPassBuilder);

        auto player = std::make_unique<Player>(engine, glm::radians(75.0f), 1600.0 / 900.0, 1e-2, 1000);
        player->position = {30, 30, 30};
        engine.set_player(std::move(player));

        auto floorModel = glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, 0, -30)), glm::vec3(4));
        auto marryModel1 = glm::scale(glm::mat4(1), glm::vec3(20));
        auto marryModel2 = glm::scale(glm::translate(glm::mat4(1), glm::vec3(40, 0, -40)), glm::vec3(10));

        auto firstPass = std::make_unique<MultiShader<3>>();
        firstPass->shaders[0] = std::make_unique<Shadow>(engine, "floor.gltf", floorModel);
        firstPass->shaders[1] = std::make_unique<Shadow>(engine, "marry.gltf", marryModel1);
        firstPass->shaders[2] = std::make_unique<Shadow>(engine, "marry.gltf", marryModel2);

        auto secondPass = std::make_unique<MultiShader<3>>();
        secondPass->shaders[0] = std::make_unique<PhongShadow>(engine, "floor.gltf", "floor", floorModel, false);
        secondPass->shaders[1] = std::make_unique<PhongShadow>(engine, "marry.gltf", "marry", marryModel1);
        secondPass->shaders[2] = std::make_unique<PhongShadow>(engine, "marry.gltf", "marry", marryModel2);

        auto mesh = std::make_unique<SubpassShader<2>>(engine.vulkan);
        mesh->shaders[0] = std::move(firstPass);
        mesh->shaders[1] = std::move(secondPass);
        engine.add_mesh(std::move(mesh));

        engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what();
    } catch (...) {
        std::cerr << "Unknow exception!";
    }

    return 0;
}
