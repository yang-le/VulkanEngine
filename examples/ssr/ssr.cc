#include <iostream>

#include "gltf.h"

namespace {
auto lightPos = glm::vec3(-2, 4, 1);
auto lightView = glm::lookAt(lightPos, glm::vec3(-1.6, 3.1, 0.8), glm::vec3(1, 0, 0));
auto lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1e-2f, 100.0f);
}  // namespace

struct PhongShadow : gltf::PrimitiveShader {
    PhongShadow(Engine& engine, const std::string& shader_file, gltf::Primitive& primitive, const glm::mat4& modelMat)
        : engine(engine), gltf::PrimitiveShader(primitive, shader_file, engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};

        write_uniform(2, modelMat);                     // model matrix
        write_uniform(3, lightProjection * lightView);  // light VP
    }

    virtual ~PhongShadow() override {
        // erase texture to avoid double free
        if (primitive.baseColorTexture) textures.erase(10);
        textures.erase(11);
    }

    virtual void init() override {
        Shader::init();

        write_uniform(4, primitive.baseColorFactor,
                      vk::ShaderStageFlagBits::eFragment);  // baseColorFactor
        write_uniform(5, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // specular
        write_uniform(6, lightPos,
                      vk::ShaderStageFlagBits::eFragment);  // light position
        write_uniform(7, glm::vec3(0),
                      vk::ShaderStageFlagBits::eFragment);  // camera position
        write_uniform(8, 1.0f,
                      vk::ShaderStageFlagBits::eFragment);  // light intensity

        if (primitive.baseColorTexture) textures[10] = *primitive.baseColorTexture;
    }

    virtual void pre_attach() override { textures[11] = engine.get_offscreen_depth_texture(); }

    virtual void update() override {
        Shader::update();

        write_uniform(7, engine.get_player().position);
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

struct Shadows : MultiShader<2> {
    Shadows(Engine& engine) : engine(engine) {}
    virtual void pre_attach() override {
        vk::AttachmentDescription depthAttachment(
            {}, vk::Format::eD16Unorm, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal);
        auto renderPassBuilder = engine.vulkan.makeRenderPassBuilder(depthAttachment, 0).addSubpass();
        renderPassBuilder.offscreenDepth = true;
        engine.vulkan.addRenderPass(renderPassBuilder);
    }

    Engine& engine;
};

struct PhongShadows : MultiShader<2> {
    PhongShadows(Engine& engine) : engine(engine) {}

    virtual void pre_attach() override {
        MultiShader<2>::pre_attach();
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

            auto player = std::make_unique<Player>(engine, glm::radians(75.0f), 1600.0 / 900.0, 1e-3, 1000);
            player->position = {6, 1, 0};
            engine.set_player(std::move(player));

            gltf::Model model(&engine.vulkan, "assets/cube/cube1.gltf");
            model.load();

            auto firstPass = std::make_unique<Shadows>(engine);
            firstPass->shaders[0] =
                std::make_unique<Shadow>(engine, model.meshes[0].primitives[0], model.nodes[0].getMatrix());
            firstPass->shaders[1] =
                std::make_unique<Shadow>(engine, model.meshes[1].primitives[0], model.nodes[1].getMatrix());
            auto secondPass = std::make_unique<PhongShadows>(engine);
            secondPass->shaders[0] = std::make_unique<PhongShadow>(engine, "floor", model.meshes[0].primitives[0],
                                                                   model.nodes[0].getMatrix());
            secondPass->shaders[1] = std::make_unique<PhongShadow>(engine, "marry", model.meshes[1].primitives[0],
                                                                   model.nodes[1].getMatrix());

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
