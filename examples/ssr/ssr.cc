#include <iostream>

#include "gltf.h"

namespace {
auto lightPos = glm::vec3(-2, 4, 1);
auto lightDir = glm::vec3(0.4, -0.9, -0.2);
auto lightRadiance = glm::vec3(1, 1, 1);

auto lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(1, 0, 0));
auto lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1e-2f, 100.0f);
}  // namespace

struct SSR : gltf::PrimitiveShader {
    SSR(Engine& engine, gltf::Primitive& primitive, const glm::mat4& modelMat)
        : engine(engine), gltf::PrimitiveShader(primitive, "ssr", engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat};

        write_uniform(2, modelMat);  // model matrix
    }

    virtual void init() override {
        Shader::init();

        write_uniform(3, -lightDir, vk::ShaderStageFlagBits::eFragment);      // light dir
        write_uniform(4, glm::vec3(0), vk::ShaderStageFlagBits::eFragment);   // camera pos
        write_uniform(5, lightRadiance, vk::ShaderStageFlagBits::eFragment);  // light radiance
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

struct SSRs : MultiSubpassShader<2> {
    SSRs(Engine& engine) : engine(engine), MultiSubpassShader<2>(engine.vulkan) {}

    virtual void pre_attach() override {
        MultiShader<2>::pre_attach();
        auto renderPassBuilder = engine.vulkan
                                     .makeRenderPassBuilder({engine.get_surface_format(), vk::Format::eR8G8B8A8Unorm,
                                                             vk::Format::eR32Sfloat, vk::Format::eR16G16B16A16Sfloat,
                                                             vk::Format::eR32Sfloat, vk::Format::eR16G16B16A16Sfloat})
                                     .addSubpass({0, 1, 2, 3, 4, 5})
                                     .addSubpass({0}, {1, 2, 3, 4, 5})
                                     .dependOn(0);
        engine.vulkan.addRenderPass(renderPassBuilder);
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
                std::make_unique<Shadow>(engine, model.meshes[1].primitives[0], model.nodes[1].getMatrix());
            firstPass->shaders[1] =
                std::make_unique<Shadow>(engine, model.meshes[0].primitives[0], model.nodes[0].getMatrix());

            auto gbufferSubPass = std::make_unique<MultiShader<2>>();
            gbufferSubPass->shaders[0] =
                std::make_unique<GBuffer>(engine, model.meshes[1].primitives[0], model.nodes[1].getMatrix());
            gbufferSubPass->shaders[1] =
                std::make_unique<GBuffer>(engine, model.meshes[0].primitives[0], model.nodes[0].getMatrix());

            auto ssrSubPass = std::make_unique<MultiShader<2>>();
            ssrSubPass->shaders[0] =
                std::make_unique<SSR>(engine, model.meshes[1].primitives[0], model.nodes[1].getMatrix());
            ssrSubPass->shaders[1] =
                std::make_unique<SSR>(engine, model.meshes[0].primitives[0], model.nodes[0].getMatrix());

            auto secondPass = std::make_unique<SSRs>(engine);
            secondPass->shaders[0] = std::move(gbufferSubPass);
            secondPass->shaders[1] = std::move(ssrSubPass);

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
