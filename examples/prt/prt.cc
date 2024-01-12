#include <fstream>
#include <iostream>

#define GLM_FORCE_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/type_aligned.hpp>

#include "gltf.h"

struct Marry : gltf::Shader {
    Marry(Engine& engine, const glm::mat4& model) : engine(engine), gltf::Shader("marry2.gltf", "prt", engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat,
                        vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat};
        write_uniform(2, model);  // model matrix
    }

    virtual void init() override {
        Shader::init();

        std::ifstream light("assets/CornellBox/light.txt");

        std::array<glm::aligned_mat3, 3> preLight;
        for (auto& l : preLight) {
            float data[9];
            for (int i = 0; i < 9; ++i) light >> data[i];
            l = glm::make_mat3(data);
        }

        write_uniform(3, preLight);

        std::ifstream transport("assets/CornellBox/transport.txt");
        uint32_t count;
        transport >> count;

        std::array<std::vector<glm::vec3>, 3> transports;
        for (auto& lt : transports) lt.reserve(count);

        for (int i = 0; i < count; ++i)
            for (auto& lt : transports) {
                float data[3];
                transport >> data[0] >> data[1] >> data[2];
                lt.push_back(glm::make_vec3(data));
            }

        for (auto& lt : transports) {
            model.bufferViews.push_back(vulkan->createGltfBuffer(lt.data(), lt.size() * sizeof(lt.front())));
            model.meshes[0].primitives[0].vertex.push_back(model.bufferViews.back().buffer);
            model.meshes[0].primitives[0].vertexOffset.push_back(0);
            model.meshes[0].primitives[0].vertexStrides.push_back(sizeof(lt.front()));
            model.meshes[0].primitives[1].vertex.push_back(model.bufferViews.back().buffer);
            model.meshes[0].primitives[1].vertexOffset.push_back(85740);
            model.meshes[0].primitives[1].vertexStrides.push_back(sizeof(lt.front()));
        }
    }

    virtual void pre_attach() override {
        auto renderPassBuilder =
            vulkan->makeRenderPassBuilder({engine.get_surface_format(), vk::Format::eD16Unorm}, true);
        vulkan->addRenderPass(renderPassBuilder);
    }

    Engine& engine;
};

struct CubeMap : Shader {
    CubeMap(Engine& engine) : engine(engine), Shader("cubemap", engine) {
        vert_formats = {vk::Format::eR32G32B32Sfloat};
    }

    virtual void init() override {
        Shader::init();
        constexpr std::array<std::tuple<float, float, float>, 8> skyboxVertices = {
            // position
            std::tuple<float, float, float>{-1.0f, 1.0f, -1.0f},
            {-1.0f, 1.0f, 1.0f},
            {1.0f, 1.0f, 1.0f},
            {1.0f, 1.0f, -1.0f},
            {-1.0f, -1.0f, -1.0f},
            {-1.0f, -1.0f, 1.0f},
            {1.0f, -1.0f, 1.0f},
            {1.0f, -1.0f, -1.0f}};
        constexpr std::array<size_t, 36> skyboxIndices = {//+x
                                                          3, 6, 7, 3, 2, 6,
                                                          //-x
                                                          0, 5, 1, 0, 4, 5,
                                                          //+y
                                                          0, 1, 2, 0, 2, 3,
                                                          //-y
                                                          4, 6, 5, 4, 7, 6,
                                                          //+z
                                                          1, 5, 6, 1, 6, 2,
                                                          //-z
                                                          0, 7, 4, 0, 3, 7};
        constexpr std::array<std::tuple<float, float, float>, 36> vertex_data = get_data(skyboxVertices, skyboxIndices);
        write_vertex(vertex_data);
        write_texture(2, {"CornellBox/posx.jpg", "CornellBox/negx.jpg", "CornellBox/posy.jpg", "CornellBox/negy.jpg",
                          "CornellBox/posz.jpg", "CornellBox/negz.jpg"});
    }

    virtual void update() override {
        auto view = camera->view;
        // Cancel out translation
        view[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        write_uniform(1, view);
    }

    virtual void pre_attach() override {
        auto renderPassBuilder = vulkan->makeRenderPassBuilder(engine.get_surface_format());
        vulkan->addRenderPass(renderPassBuilder);
    }

    Engine& engine;
};

int main(int argc, char* argv[]) {
    try {
        Engine engine(1600, 900, 2);

        auto player = std::make_unique<Player>(engine, glm::radians(75.0f), 1600.0 / 900.0, 0.1, 1000);
        player->position = {-20, 180, 250};

        engine.set_player(std::move(player));

        auto mesh = std::make_unique<MultiPassShader<2>>(engine.vulkan);
        mesh->shaders[0] = std::make_unique<CubeMap>(engine);
        mesh->shaders[1] = std::make_unique<Marry>(engine, glm::scale(glm::mat4(1), glm::vec3(52)));
        engine.add_mesh(std::move(mesh));

        engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what();
    } catch (...) {
        std::cerr << "Unknow exception!";
    }

    return 0;
}
